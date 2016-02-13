///////////////////////////////////////////////////////////////////////////////////////
//
// p_AsyncFilesys.cpp		GRJ 11 Oct 2002
//
// Asynchronous file system
//
///////////////////////////////////////////////////////////////////////////////////////

#include <sys/config/config.h>

#include <sys/file/ngps/p_AsyncFilesys.h>
#include <sys/file/ngps/hed.h>
#include <sys/file/ngps/FileIO/FileIO.h>

#include <eetypes.h>
#include <eekernel.h>
#include <sif.h>
#include <sifrpc.h>

namespace File
{

// Debug info
#define SHOW_ASYNC_INFO	0

#if SHOW_ASYNC_INFO
#define ShowAsyncInfo(A...) scePrintf(##A)
#else
#define ShowAsyncInfo(A...) 
#endif

extern SHed *gpHed;		// Made global for p_AsyncFilesystem
extern unsigned int gWadLSN;

///////////////////////////////////////////////////////////////////////////////////////

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2AsyncFileHandle::CPs2AsyncFileHandle()
{
	m_file_handle_index = -1;
	m_num_open_requests = 0;
	mp_non_aligned_buffer = NULL;
	mp_temp_aligned_buffer = NULL;
	mp_temp_aligned_buffer_base = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2AsyncFileHandle::~CPs2AsyncFileHandle()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileHandle::io_callback(EAsyncFunctionType function, int result, uint32 data)
{
	SFileIOResultPacket *p_packet = (SFileIOResultPacket *) data;
	int return_data = p_packet->m_return_data;

	// Do any special function processing
	switch (function)
	{
	case FUNC_OPEN:
		m_file_handle_index = result;		// Save IOP file handle index
		if (result < 0)
		{
			//Dbg_MsgAssert(0, ("Error: Async open returned %d", result));
			m_file_size = 0;
		} else {
			m_file_size = return_data;
			ShowAsyncInfo("io_callback: Open returned handle index %d with filesize %d\n", result, return_data);
		}
		break;

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
			ShowAsyncInfo("Copying from %x to %x of size %d", mp_temp_aligned_buffer, mp_non_aligned_buffer, result);
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
				CPs2AsyncFileLoader::sIAddBufferToFreeList(mp_temp_aligned_buffer_base);
				mp_non_aligned_buffer = NULL;
				mp_temp_aligned_buffer = NULL;
				mp_temp_aligned_buffer_base = NULL;
			}
		}
		break;

	case FUNC_CLOSE:
		if (get_busy_count() > 1)
		{
			scePrintf("********* ERROR: Still other %d requests open after close completed on handle %x and request id %d\n", get_busy_count() - 1, this, p_packet->m_header.opt);
			//while (1)
			//	;
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
	CAsyncFileHandle::io_callback(function, result, return_data);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileHandle::non_cache_aligned_data_copy(SFileIOResultPacket *p_result, bool last_packet)
{
	Dbg_Assert(p_result->m_non_aligned_data_size > 0);
	
	int request_id = p_result->m_header.opt;
	const SRequest *p_request = get_request(request_id);
	Dbg_Assert(p_request);
	Dbg_Assert(p_request->mp_buffer);

	// Check if it goes at the start or end
	if (last_packet)
	{
		// end of buffer
		memcpy((void *) ((uint32) p_request->mp_buffer + (p_request->m_buffer_size - p_result->m_non_aligned_data_size)),
			   p_result->m_non_aligned_data, p_result->m_non_aligned_data_size);
	}
	else
	{
		// beginning of buffer
		memcpy(p_request->mp_buffer, p_result->m_non_aligned_data, p_result->m_non_aligned_data_size);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CPs2AsyncFileHandle::add_request_id(int request_id, EAsyncFunctionType function, SFileIORequestPacket *p_packet)
{
	if (m_num_open_requests >= NUM_REQUESTS)
	{
		Dbg_MsgAssert(0, ("Too many open requests for file handle"));
		return false;
	}

	// We must disable interrupts here so that the IO interrupts don't change things underneath us.
	DI();

	// Save buffer info if it is a read
	if (p_packet->m_request.m_command == FILEIO_READ)
	{
		m_open_requests[m_num_open_requests].mp_buffer = (uint8 *) p_packet->m_request.m_param.m_rw.m_buffer;
		m_open_requests[m_num_open_requests].m_buffer_size = p_packet->m_request.m_param.m_rw.m_size;
	}
	else
	{
		m_open_requests[m_num_open_requests].mp_buffer = NULL;
		m_open_requests[m_num_open_requests].m_buffer_size = 0;
	}

	m_open_requests[m_num_open_requests].m_request_id = request_id;
	m_open_requests[m_num_open_requests++].m_function = function;

	// And re-enable interrupts
	EI();

	//scePrintf("Adding request %d to entry #%d for %x with busy %d\n", request_id, (m_num_open_requests-1), this, get_busy_count());

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EAsyncFunctionType	CPs2AsyncFileHandle::get_request_function(int request_id) const
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

const CPs2AsyncFileHandle::SRequest *	CPs2AsyncFileHandle::get_request(int request_id) const
{
	// Find request id in list
	for (int i = 0; i < m_num_open_requests; i++)
	{
		if (m_open_requests[i].m_request_id == request_id)
		{
			return &(m_open_requests[i]);
		}
	}

	// Request not found
	Dbg_MsgAssert(0, ("Can't find request %d on %x", request_id, this));
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CPs2AsyncFileHandle::clear_request_id(int request_id)
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

void				CPs2AsyncFileHandle::plat_init()
{
	m_file_handle_index = -1;
	m_num_open_requests = 0;
	mp_non_aligned_buffer = NULL;
	mp_temp_aligned_buffer = NULL;
	mp_temp_aligned_buffer_base = NULL;
}

volatile bool		CPs2AsyncFileHandle::plat_is_done()
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	return false;
}

volatile bool		CPs2AsyncFileHandle::plat_is_busy()
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	return false;
}

bool				CPs2AsyncFileHandle::plat_is_eof() const
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileHandle::plat_set_priority( int priority )
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	// Now send the packet
	SFileIORequestPacket set_packet __attribute__ ((aligned(16)));

	set_packet.m_request.m_command						= FILEIO_SET;
	set_packet.m_request.m_param.m_set.m_file_handle	= m_file_handle_index;
	set_packet.m_request.m_param.m_set.m_variable		= FILEIO_VAR_PRIORITY;
	set_packet.m_request.m_param.m_set.m_value			= priority;

	CFileIOManager::sSendCommand(&set_packet, this, true);

	if (m_blocking)
	{
		WaitForIO();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileHandle::plat_set_stream( bool stream )
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	// Now send the packet
	SFileIORequestPacket set_packet __attribute__ ((aligned(16)));

	set_packet.m_request.m_command						= FILEIO_SET;
	set_packet.m_request.m_param.m_set.m_file_handle	= m_file_handle_index;
	set_packet.m_request.m_param.m_set.m_variable		= FILEIO_VAR_STREAM;
	set_packet.m_request.m_param.m_set.m_value			= stream;

	CFileIOManager::sSendCommand(&set_packet, this, true);

	if (m_blocking)
	{
		WaitForIO();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileHandle::plat_set_destination( EAsyncMemoryType destination )
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	// Now send the packet
	SFileIORequestPacket set_packet __attribute__ ((aligned(16)));

	set_packet.m_request.m_command						= FILEIO_SET;
	set_packet.m_request.m_param.m_set.m_file_handle	= m_file_handle_index;
	set_packet.m_request.m_param.m_set.m_variable		= FILEIO_VAR_DESTINATION;
	set_packet.m_request.m_param.m_set.m_value			= destination;

	CFileIOManager::sSendCommand(&set_packet, this, true);

	if (m_blocking)
	{
		WaitForIO();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileHandle::plat_set_buffer_size( size_t buffer_size )
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	// Now send the packet
	SFileIORequestPacket set_packet __attribute__ ((aligned(16)));

	set_packet.m_request.m_command						= FILEIO_SET;
	set_packet.m_request.m_param.m_set.m_file_handle	= m_file_handle_index;
	set_packet.m_request.m_param.m_set.m_variable		= FILEIO_VAR_BUFFER_SIZE;
	set_packet.m_request.m_param.m_set.m_value			= buffer_size;

	CFileIOManager::sSendCommand(&set_packet, this, true);

	if (m_blocking)
	{
		WaitForIO();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileHandle::plat_set_blocking( bool block )
{
	// IOP doesn't need to be notified
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

size_t				CPs2AsyncFileHandle::plat_load(void *p_buffer)
{
	// This function will just get the file size and do one big read
	// It is not truly asynchronous now because of the seek wait
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	if (m_position != 0)
	{
		ShowAsyncInfo("plat_load: Doing seek.\n");
		// Now put at the beginning
		Seek(0, SEEK_SET);
		WaitForIO();

		m_current_function = FUNC_LOAD;
	}

	ShowAsyncInfo("plat_load: Reading into buffer %x.\n", p_buffer);
	// And do the read
	return plat_read(p_buffer, 1, GetFileSize());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

size_t				CPs2AsyncFileHandle::plat_read(void *p_buffer, size_t size, size_t count)
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));
	//Dbg_MsgAssert((((uint32) p_buffer) & 0xF) == 0, ("Buffer %x needs to be quadword aligned", p_buffer));

	// Now send the packet
	SFileIORequestPacket read_packet __attribute__ ((aligned(16)));

	uint32 non_aligned_cache_start_bytes = 0;
	uint32 non_aligned_cache_end_bytes = 0;

	// Read in non-aligned bytes first (this is a temp hack)
	if (m_mem_destination == MEM_EE)
	{
		// only look for 16-byte alignment here
		bool non_aligned_bytes = ((uint32) p_buffer) & 0xF;

		// Also check to see if we aren't cache aligned on either end of a small buffer, since we assume the
		// cache alignment problem is at the end of a small buffer.
		int non_aligned_cache_bytes = ((uint32) p_buffer) & FILEIO_CACHE_BLOCK_NONALIGN_MASK;
		if (non_aligned_cache_bytes)
		{
			non_aligned_cache_bytes = FILEIO_CACHE_BLOCK_SIZE - non_aligned_cache_bytes;
		}
		//bool non_aligned_cache_size = ((size * count) - non_aligned_cache_bytes) & FILEIO_CACHE_BLOCK_NONALIGN_MASK;

		if (non_aligned_bytes || ((non_aligned_cache_bytes /*|| non_aligned_cache_size*/) && (size * count) < FILEIO_BUFFER_SIZE))
		{
			Dbg_MsgAssert(!mp_temp_aligned_buffer_base, ("Handling two small reads in succession not implemented yet"));

			int aligned_size = ((size * count) + (2 * (FILEIO_CACHE_BLOCK_SIZE - 1))) & FILEIO_CACHE_BLOCK_ALIGN_MASK;
			mp_non_aligned_buffer = (uint8 *) p_buffer;
			mp_temp_aligned_buffer_base = CPs2AsyncFileLoader::sGetBuffer(aligned_size);
			mp_temp_aligned_buffer = (uint8 *) (((uint32) mp_temp_aligned_buffer_base + (FILEIO_CACHE_BLOCK_SIZE - 1)) & FILEIO_CACHE_BLOCK_ALIGN_MASK);
			p_buffer = mp_temp_aligned_buffer;
		}
		else
		{
			// Now check for cache-alignment problems that will be handled with the messages
			non_aligned_cache_start_bytes = (((uint32) p_buffer) & FILEIO_CACHE_BLOCK_NONALIGN_MASK);
			if (non_aligned_cache_start_bytes)
			{
				non_aligned_cache_start_bytes = FILEIO_CACHE_BLOCK_SIZE - non_aligned_cache_start_bytes;
			}
			non_aligned_cache_end_bytes = ((size * count) - non_aligned_cache_start_bytes) & FILEIO_CACHE_BLOCK_NONALIGN_MASK;
		}
	}

	read_packet.m_request.m_command								 = FILEIO_READ;
	read_packet.m_request.m_param.m_rw.m_file_handle			 = m_file_handle_index;
	read_packet.m_request.m_param.m_rw.m_buffer					 = p_buffer;
	read_packet.m_request.m_param.m_rw.m_size					 = size * count;
	read_packet.m_request.m_param.m_rw.m_non_aligned_start_bytes = non_aligned_cache_start_bytes;
	read_packet.m_request.m_param.m_rw.m_non_aligned_end_bytes	 = non_aligned_cache_end_bytes;

	// Mark as busy before sending command
	inc_busy_count();

	// Garrett (4/18/03): According to the Sony Newsgroups, a FlushCache is not needed if only 1 packet is sent.
	// Only the additional packet needs the flush.  I think this was helping the issue of non-64 byte aligned
	// read buffers.  We should try taking this out after the buffers are aligned.
	FlushCache(WRITEBACK_DCACHE);		// This one is needed, but I think invalidating the cache will work, instead

	CFileIOManager::sSendCommand(&read_packet, this, true);

	Dbg_Assert(((non_aligned_cache_start_bytes == 0) || (non_aligned_cache_end_bytes == 0)) || ((size * count) >= FILEIO_BUFFER_SIZE));

	if (m_blocking)
	{
		return WaitForIO();
	} else {
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

size_t				CPs2AsyncFileHandle::plat_write(void *p_buffer, size_t size, size_t count)
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));
	Dbg_MsgAssert((((uint32) p_buffer) & 0xF) == 0, ("Buffer %x needs to be quadword aligned", p_buffer));

	// Now send the packet
	SFileIORequestPacket write_packet __attribute__ ((aligned(16)));

	write_packet.m_request.m_command					= FILEIO_WRITE;
	write_packet.m_request.m_param.m_rw.m_file_handle	= m_file_handle_index;
	write_packet.m_request.m_param.m_rw.m_buffer		= p_buffer;
	write_packet.m_request.m_param.m_rw.m_size			= size * count;

	// Mark as busy before sending command
	inc_busy_count();

	FlushCache(WRITEBACK_DCACHE);		// This one is needed, but I think invalidating the cache will work, instead

	int req_id = CFileIOManager::sSendCommand(&write_packet, this, true);

	// Wait for result
	CFileIOManager::sWaitRequestCompletion(req_id);

	return CFileIOManager::sGetRequestResult(write_packet.m_header.opt);
}

char *				CPs2AsyncFileHandle::plat_get_s(char *p_buffer, int maxlen)
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					CPs2AsyncFileHandle::plat_seek(long offset, int origin)
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	// Now send the packet
	SFileIORequestPacket seek_packet __attribute__ ((aligned(16)));

	seek_packet.m_request.m_command						= FILEIO_SEEK;
	seek_packet.m_request.m_param.m_seek.m_file_handle	= m_file_handle_index;
	seek_packet.m_request.m_param.m_seek.m_offset		= offset;
	seek_packet.m_request.m_param.m_seek.m_origin		= origin;

	// Mark as busy before sending command
	inc_busy_count();

	CFileIOManager::sSendCommand(&seek_packet, this, true);

	if (m_blocking)
	{
		return WaitForIO();
	} else {
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CPs2AsyncFileHandle::plat_open(const char *filename)
{
	// Init vars
	m_position = 0;

	// Now send the packet
	const char *p_continued_filename = NULL;
	SFileIORequestPacket open_packet __attribute__ ((aligned(16)));

	open_packet.m_request.m_command											= FILEIO_OPEN;
	if (Config::CD())
	{
		if ( !gpHed )
		{
			return ( false );
		}
		SHedFile *tempHed = FindFileInHed( filename, gpHed );
		if ( !tempHed )
		{
			//Dbg_MsgAssert( 0,( "File %s cannot be found in Hed file.", filename ));
			Dbg_Message( "File %s cannot be found in Hed file.", filename );
			return ( false );
		}

		m_file_size = tempHed->GetFileSize();

		open_packet.m_request.m_param.m_open.m_file.m_raw_name.m_sector_number	= gWadLSN + (tempHed->Offset / SECTOR_SIZE);
		open_packet.m_request.m_param.m_open.m_file.m_raw_name.m_file_size		= m_file_size;
		open_packet.m_request.m_param.m_open.m_host_read 						= FILEIO_HOSTOPEN_FALSE;

		ShowAsyncInfo("Sending open command for file %s of size %d and offset %d at sector %d\n", filename, m_file_size, tempHed->Offset, gWadLSN + (tempHed->Offset / SECTOR_SIZE));
	} else {
		//Dbg_MsgAssert(strlen(filename) < FILEIO_MAX_FILENAME_SIZE, ("Can't open file %s: name longer than %d chars", filename, FILEIO_MAX_FILENAME_SIZE - 1));

		// Copy filename and check if we need to insert a "host0:" into the name
		uint chars_copied = FILEIO_MAX_FILENAME_SIZE - 1;
		if (strchr(filename, ':'))
		{
			strncpy(open_packet.m_request.m_param.m_open.m_file.m_filename, filename, chars_copied);
		} else {
			//Dbg_MsgAssert(strlen(filename) < (FILEIO_MAX_FILENAME_SIZE - 5), ("Can't open file %s: name longer than %d chars", filename, FILEIO_MAX_FILENAME_SIZE - 6));
			chars_copied -= 5;
			strcpy(open_packet.m_request.m_param.m_open.m_file.m_filename, "host:");
			strncpy(&open_packet.m_request.m_param.m_open.m_file.m_filename[5], filename, chars_copied);
		}

		// Check for long filename
		if (chars_copied < strlen(filename))
		{
			p_continued_filename = &(filename[chars_copied]);
			open_packet.m_request.m_param.m_open.m_host_read = FILEIO_HOSTOPEN_EXTEND_NAME;
		}
		else
		{
			open_packet.m_request.m_param.m_open.m_host_read = FILEIO_HOSTOPEN_TRUE;
		}


		ShowAsyncInfo("Sending host open command for file %s\n", filename);
	}
	open_packet.m_request.m_param.m_open.m_memory_dest 						= m_mem_destination;
	open_packet.m_request.m_param.m_open.m_buffer_size 						= m_buffer_size;
	open_packet.m_request.m_param.m_open.m_priority 						= m_priority;
	open_packet.m_request.m_param.m_open.m_attributes						= 0;		// need to define this

	// Mark as busy before sending command
	inc_busy_count();

	uint request_id = CFileIOManager::sSendCommand(&open_packet, this, true);

	if (p_continued_filename)
	{
		Dbg_MsgAssert(strlen(p_continued_filename) < FILEIO_MAX_FILENAME_SIZE, ("Can't open file %s: name longer than %d chars", filename, (FILEIO_MAX_FILENAME_SIZE - 1) * 2));
		strcpy(open_packet.m_request.m_param.m_open.m_file.m_filename, p_continued_filename);

		open_packet.m_request.m_command	= FILEIO_CONT;

		#ifdef	__NOPT_ASSERT__
		uint cont_request_id = 
		#endif
		CFileIOManager::sSendCommand(&open_packet, this, true, request_id);
		Dbg_Assert(cont_request_id == request_id);
	}

	if (m_blocking)
	{
		return WaitForIO() >= 0;
	} else {
		// Copy request ID into file handle to use as temp file handle
		m_file_handle_index = (int) (FILEIO_TEMP_HANDLE_FLAG | request_id);
		return true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CPs2AsyncFileHandle::raw_open(unsigned int lsn, bool blocking, int priority)
{
	Dbg_MsgAssert(!IsBusy(), ("Can't open raw file %d: asynchronous file handle already busy.", lsn));
	Dbg_MsgAssert(!mp_pre_file, ("Can't open raw file %d: already open as a PRE file.", lsn));

	// Make sure we are using the CD
	if (!Config::CD())
	{
		Dbg_MsgAssert(0, ("Raw Open not supported in host mode"));
		return false;
	}

	m_priority = priority;
	m_blocking = blocking;

	m_current_function = FUNC_OPEN;

	// Init vars
	m_position = 0;

	// Now send the packet
	SFileIORequestPacket open_packet __attribute__ ((aligned(16)));

	// Make it the max file size since a the WADs don't keep track of how big they are
	m_file_size = MAX_FILE_SIZE;

	open_packet.m_request.m_command											= FILEIO_OPEN;
	open_packet.m_request.m_param.m_open.m_file.m_raw_name.m_sector_number	= lsn;
	open_packet.m_request.m_param.m_open.m_file.m_raw_name.m_file_size		= m_file_size;
	open_packet.m_request.m_param.m_open.m_host_read 						= FILEIO_HOSTOPEN_FALSE;
	open_packet.m_request.m_param.m_open.m_memory_dest 						= m_mem_destination;
	open_packet.m_request.m_param.m_open.m_buffer_size 						= m_buffer_size;
	open_packet.m_request.m_param.m_open.m_priority 						= m_priority;
	open_packet.m_request.m_param.m_open.m_attributes						= 0;		// need to define this

	ShowAsyncInfo("Sending raw open command for file of size %d at sector %d\n", m_file_size, lsn);

	// Mark as busy before sending command
	inc_busy_count();

	uint request_id = CFileIOManager::sSendCommand(&open_packet, this, true);

	if (m_blocking)
	{
		return WaitForIO() >= 0;
	} else {
		// Copy request ID into file handle to use as temp file handle
		m_file_handle_index = (int) (FILEIO_TEMP_HANDLE_FLAG | request_id);
		return true;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CPs2AsyncFileHandle::plat_close()
{
	//Dbg_MsgAssert(m_file_handle_index != -1, ("Invalid IOP file handle index: %d", m_file_handle_index));

	// Now send the packet
	SFileIORequestPacket close_packet __attribute__ ((aligned(16)));

	close_packet.m_request.m_command						= FILEIO_CLOSE;
	close_packet.m_request.m_param.m_close.m_file_handle	= m_file_handle_index;

	// Mark as busy before sending command
	inc_busy_count();

	CFileIOManager::sSendCommand(&close_packet, this, true);

	m_file_handle_index = -1;

	if (m_blocking)
	{
		return WaitForIO();
	} else {
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////

CPs2AsyncFileLoader::SAlignBuffer	CPs2AsyncFileLoader::s_buffer_list[BUFFER_LIST_SIZE];
//volatile int		CPs2AsyncFileLoader::s_free_buffer_list_size = 0;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAsyncFileHandle *	CPs2AsyncFileLoader::sRawOpen( unsigned int lsn, bool blocking, int priority )
{
	CPs2AsyncFileHandle *p_file_handle = static_cast<CPs2AsyncFileHandle *>(s_get_file_handle());

	if (p_file_handle)
	{
		if (!p_file_handle->raw_open(lsn, blocking, priority))
		{
			//Dbg_MsgAssert(0, ("Error opening Async file %s", filename));
			s_free_file_handle(p_file_handle);
			return NULL;
		}

		return p_file_handle;
	}
	else
	{
		Dbg_MsgAssert(0, ("Out of Async handles"));
		return NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8 *				CPs2AsyncFileLoader::sGetBuffer( uint size )
{
#if 0
	if (size > ALIGN_BUFFER_SIZE)
	{
		Dbg_Message("sGetBuffer() is trying to allocate %d.  Max buffer size is %d", size, ALIGN_BUFFER_SIZE);
		Dbg_MsgAssert(0, ("sGetBuffer() is trying to allocate %d.  Max buffer size is %d", size, ALIGN_BUFFER_SIZE));
	}

	if (s_free_buffer_list_size == 0)
	{
		Dbg_Message("sGetBuffer() out of buffers");
		Dbg_MsgAssert(0, ("sGetBuffer() out of buffers"));
	}
#endif

	//Dbg_Message("***** Getting async buffer of size %d", size);

	volatile uint8 *p_buffer = NULL;
	// Make sure interrupts are off
	DI();
	for (int i = 0; i < BUFFER_LIST_SIZE; i++)
	{
		if ((s_buffer_list[i].m_size >= size) && !s_buffer_list[i].m_used)
		{
			p_buffer = s_buffer_list[i].mp_buffer;
			s_buffer_list[i].m_used = true;
		}
	}
	EI();

	Dbg_MsgAssert(p_buffer, ("Couldn't allocate align buffer of size %d", size));

	return (uint8 *) p_buffer;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileLoader::sAddBufferToFreeList( uint8 *p_dealloc_buffer )
{
	// Make sure interrupts are off
	DI();

	sIAddBufferToFreeList(p_dealloc_buffer);

	EI();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileLoader::sIAddBufferToFreeList( uint8 *p_dealloc_buffer )
{
	//scePrintf("***** Freeing async buffer\n");

#if 0 //def __NOPT_ASSERT__
	if (s_free_buffer_list_size >= FREE_LIST_SIZE)
	{
		scePrintf("CPs2AsyncFileLoader::sIAddBufferToFreeList(): Free list full.  Tell Garrett.\n");
		while (s_free_buffer_list_size >= FREE_LIST_SIZE)		// We are basically stuck here
			;
	}
#endif // __NOPT_ASSERT__

	for (int i = 0; i < BUFFER_LIST_SIZE; i++)
	{
		if (s_buffer_list[i].mp_buffer == p_dealloc_buffer)
		{
#ifdef __NOPT_ASSERT__
			if (!s_buffer_list[i].m_used)
			{
				scePrintf("CPs2AsyncFileLoader::sIAddBufferToFreeList(): Freeing a buffer that is already free.  Tell Garrett.\n");
				while (1)
					;
			}
#endif // __NOPT_ASSERT__

			s_buffer_list[i].m_used = false;
			return;
		}
	}

#ifdef __NOPT_ASSERT__
	scePrintf("CPs2AsyncFileLoader::sIAddBufferToFreeList(): Pointer wrong value.  Tell Garrett.\n");
	while (1)
		;
#endif // __NOPT_ASSERT__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileLoader::sAllocateFreeListBuffers( )
{
	int size_chunk = (ALIGN_MAX_BUFFER_SIZE - ALIGN_MIN_BUFFER_SIZE) / (BUFFER_LIST_SIZE - 1);
	for (int i = 0; i < BUFFER_LIST_SIZE; i++)
	{
		int buffer_size = ALIGN_MIN_BUFFER_SIZE + (size_chunk * i);
		s_buffer_list[i].mp_buffer = new uint8[buffer_size];
		s_buffer_list[i].m_size = buffer_size;
		s_buffer_list[i].m_used = false;
		
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CPs2AsyncFileLoader::sDeallocateFreeListBuffers( )
{
	Dbg_Assert(0);		// We shouldn't be calling this now

#if 0 
	// Make sure interrupts are off
	DI();

	for (int i = 0; i < s_free_buffer_list_size; i++)
	{
		delete [] s_free_buffer_list[i];
		s_free_buffer_list[i] = NULL;
	}
	s_free_buffer_list_size = 0;

	EI();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

sceSifCmdData	CFileIOManager::s_cmdbuffer[FILEIO_NUM_COMMAND_HANDLERS];

int				CFileIOManager::s_cur_request_id = 0;

CFileIOManager::SOpenRequest	CFileIOManager::s_open_requests[NUM_REQUESTS];
int								CFileIOManager::s_free_request_index = 0;

/******************************************************************/
/*                                                                */
/*   CALLBACK                                                     */
/*                                                                */
/******************************************************************/

void 				CFileIOManager::s_result_callback(void *p, void *q)
{
	SFileIOResultPacket *h = (SFileIOResultPacket *) p;
	int request_id = h->m_header.opt;

	SOpenRequest *p_request = s_find_request(request_id);
	Dbg_MsgAssert(p_request, ("Can't find request id # %d", request_id));

	p_request->m_result = h->m_return_value;
	p_request->m_done = h->m_done;

	//scePrintf( "Got IOP request %d result value %d back with done = %d\n", request_id, p_request->m_result, p_request->m_done );

	// Call file handle callback if request is completed
	if (p_request->mp_cur_file)
	{
		// Check if we got any non-aligned data back
		if (h->m_non_aligned_data_size > 0)
		{
			p_request->mp_cur_file->non_cache_aligned_data_copy(h, p_request->m_done);
		}

		if (p_request->m_done)
		{
			//p_request->mp_cur_file->io_callback(p_request->mp_cur_file->get_request_function(request_id), p_request->m_result, h->m_return_data);
			p_request->mp_cur_file->io_callback(p_request->mp_cur_file->get_request_function(request_id), p_request->m_result, (uint32) h);
			p_request->mp_cur_file->clear_request_id(request_id);
		}
	}

	ShowAsyncInfo( "Got IOP request %d result value %d back with done = %d\n", request_id, p_request->m_result, p_request->m_done );
	
	ExitHandler();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFileIOManager::SOpenRequest *		CFileIOManager::s_find_request(int request_id)
{
	// Start at the last used index, probably the one used
	int index = s_prev_open_index(s_free_request_index);

	SOpenRequest *p_start_request = &(s_open_requests[index]);
	SOpenRequest *p_request = p_start_request;

	do
	{
		if (p_request->m_id == request_id)
		{
			return p_request;
		}
		
		index = s_prev_open_index(index);
		p_request = &(s_open_requests[index]);
	} while (p_request != p_start_request);

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CFileIOManager::sInit()
{
	// Communicate with the IOP side
    sceSifInitRpc(0);
	DI();

	ShowAsyncInfo( "Setting up Sif Cmd with FileIO IRX...\n" );
	sceSifSetCmdBuffer(&s_cmdbuffer[0], FILEIO_NUM_COMMAND_HANDLERS);
	sceSifAddCmdHandler(FILEIO_RESULT_COMMAND, s_result_callback, NULL);

	EI();

	// Clear out pending requests
	for (int i = 0; i < NUM_REQUESTS; i++)
	{
		s_open_requests[i].m_done = true;
		s_open_requests[i].m_id = -1;
		s_open_requests[i].m_result = -1;
	}
	s_free_request_index = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					CFileIOManager::sSendCommand(SFileIORequestPacket *p_packet, CPs2AsyncFileHandle *p_file, bool wait_for_send, int continuation_request)
{
	SOpenRequest *p_request;

	if (continuation_request >= 0)
	{
		p_request = s_find_request(continuation_request);
		Dbg_MsgAssert(p_request, ("Can't find open request %d to continue", continuation_request));

		// Just in case they didn't wait before
		sWaitSendCompletion(p_request->m_id);
	}
	else
	{
		p_request = &(s_open_requests[s_free_request_index]);
		Dbg_MsgAssert(p_request->m_done, ("Too many open requests"));
		s_free_request_index = s_next_open_index(s_free_request_index);

		int request_id = s_cur_request_id++;

		p_request->m_done		= false;		// Callback will change it to true
		p_request->m_id			= request_id;
		p_request->mp_cur_file	= p_file;
		if (p_file)
		{
			p_file->add_request_id(request_id, p_file->m_current_function, p_packet);
		}

		p_packet->m_header.opt = request_id;
	}

	p_request->m_sif_cmd_id = sceSifSendCmd(FILEIO_REQUEST_COMMAND, p_packet, sizeof(SFileIORequestPacket), 0, 0, 0);

	if (wait_for_send)
	{
		sWaitSendCompletion(p_request->m_id);
	}

	return p_request->m_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CFileIOManager::sWaitSendCompletion(int request_id)
{
	SOpenRequest *p_request = s_find_request(request_id);
	Dbg_MsgAssert(p_request, ("Can't find request id # %d", request_id));

	while(sceSifDmaStat(p_request->m_sif_cmd_id) >= 0)
		;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CFileIOManager::sWaitRequestCompletion(int request_id)
{
	SOpenRequest *p_request = s_find_request(request_id);
	Dbg_MsgAssert(p_request, ("Can't find request id # %d", request_id));

	while (!p_request->CommandDone())
		;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

volatile bool		CFileIOManager::sCommandDone(int request_id)
{
	SOpenRequest *p_request = s_find_request(request_id);
	Dbg_MsgAssert(p_request, ("Can't find request id # %d", request_id));

	return p_request->m_done;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					CFileIOManager::sGetRequestResult(int request_id)
{
	SOpenRequest *p_request = s_find_request(request_id);
	Dbg_MsgAssert(p_request, ("Can't find request id # %d", request_id));

	return p_request->m_result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileLoader::s_plat_init()
{
	for (int i = 0; i < MAX_FILE_HANDLES; i++)
	{
		if (s_file_handles[i])
		{
			s_file_handles[i]->init();
		}
		else
		{
			s_file_handles[i] = new CPs2AsyncFileHandle;
		}
	}

	// Allocate EE align buffers
	CPs2AsyncFileLoader::sAllocateFreeListBuffers();

	// Init the FileIO Manager
	CFileIOManager::sInit();

	// Now send the first packet
	SFileIORequestPacket init_packet __attribute__ ((aligned(16)));

	init_packet.m_request.m_command						= FILEIO_INIT;
	init_packet.m_request.m_param.m_init.m_buffer_size	= DEFAULT_BUFFER_SIZE;
	init_packet.m_request.m_param.m_init.m_priority		= DEFAULT_ASYNC_PRIORITY;
	init_packet.m_request.m_param.m_init.m_memory_dest	= DEFAULT_MEMORY_TYPE;
	init_packet.m_request.m_param.m_init.m_init_cd_device = Config::CD();

	int req_id = CFileIOManager::sSendCommand(&init_packet, NULL, true);

	Dbg_Message("Sending first init command to fileio.irx: waiting for completion");

	// Wait for result (no async inits)
	CFileIOManager::sWaitRequestCompletion(req_id);

	Dbg_Message("Sent first init command to fileio.irx.");

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileLoader::s_plat_cleanup()
{
	for (int i = 0; i < MAX_FILE_HANDLES; i++)
	{
		if (s_file_handles[i])
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

bool				CAsyncFileLoader::s_plat_async_supported()
{
	// Works on the PS2
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CAsyncFileLoader::s_plat_exist(const char *filename)
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileLoader::s_plat_update(void)
{
	//// Free any buffers let go during an interrupt
	//CPs2AsyncFileLoader::sDeallocateFreeListBuffers();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileLoader::s_plat_swap_callback_list()
{
	// We must disable interrupts here so that the IO interrupts don't change things underneath us.
	DI();

	s_cur_callback_list_index = s_cur_callback_list_index ^ 1;	// Swap Indexes
	s_new_io_completion = false;								// And clear flag

#ifdef __NOPT_ASSERT__
	bool new_list_clear = s_num_callbacks[s_cur_callback_list_index] == 0;
#endif // __NOPT_ASSERT__

	// And re-enable interrupts
	EI();

	Dbg_MsgAssert(new_list_clear, ("Async IO swapped callback list isn't empty"));
}

}



