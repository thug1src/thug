/*****************************************************************************
**																			**
**			              Neversoft Entertainment	                        **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Sys Library												**
**																			**
**	Module:			File													**
**																			**
**	Created:		10/11/02	-	grj										**
**																			**
**	File name:		core/sys/p_AsyncFilesys.h									**
**																			**
*****************************************************************************/

#ifndef	__SYS_FILE_P_ASYNC_FILESYS_H
#define	__SYS_FILE_P_ASYNC_FILESYS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sys/file/AsyncFilesys.h>
#include <sys/file/ngps/FileIO/FileIO.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace File
{



/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

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

// Forward declarations
class CFileIOManager;
class CPs2AsyncFileLoader;

/////////////////////////////////////////////////////////////////////////////////////
// The asynchronous file class
//
class CPs2AsyncFileHandle : public CAsyncFileHandle
{
public:

protected:

						CPs2AsyncFileHandle();
	virtual 			~CPs2AsyncFileHandle();

private:
	// Constants
	enum
	{
		NUM_REQUESTS = 16
	};

	// Request data
	struct SRequest
	{
		int					m_request_id;
		EAsyncFunctionType	m_function;
		uint8 *				mp_buffer;
		int					m_buffer_size;
	};

	// Callback functions
	virtual void		io_callback(EAsyncFunctionType function, int result, uint32 data);

	volatile int		m_file_handle_index;

	// Open requests
	SRequest		  	m_open_requests[NUM_REQUESTS];
	volatile int		m_num_open_requests;

	// Non-aligned buffer IO
	uint8 *				mp_non_aligned_buffer;
	uint8 *				mp_temp_aligned_buffer;
	uint8 *				mp_temp_aligned_buffer_base;

	bool				add_request_id(int request_id, EAsyncFunctionType function, SFileIORequestPacket *p_packet);
	EAsyncFunctionType	get_request_function(int request_id) const;
	const SRequest *	get_request(int request_id) const;
	bool				clear_request_id(int request_id);

	// PS2 only IO calls
	bool				raw_open(unsigned int lsn, bool blocking, int priority);

	// PS2 internal calls
	void				non_cache_aligned_data_copy(SFileIOResultPacket *p_result, bool last_packet);

	// platform-specific calls
	virtual void		plat_init( void );

	virtual bool		plat_open( const char *filename );
	virtual bool		plat_close( void );

	virtual volatile bool	plat_is_done( void );
	virtual volatile bool	plat_is_busy( void );
	virtual bool		plat_is_eof( void ) const;

	virtual void		plat_set_priority( int priority );
	virtual void		plat_set_stream( bool stream );
	virtual void		plat_set_destination( EAsyncMemoryType destination );
	virtual void		plat_set_buffer_size( size_t buffer_size );
	virtual void		plat_set_blocking( bool block );

	virtual size_t		plat_load( void *p_buffer );
	virtual size_t		plat_read( void *p_buffer, size_t size, size_t count );
	virtual size_t		plat_write( void *p_buffer, size_t size, size_t count );
	virtual char *		plat_get_s( char *p_buffer, int maxlen );
	virtual int			plat_seek( long offset, int origin );

	// Friends
	friend CAsyncFileLoader;
	friend CPs2AsyncFileLoader;
	friend CFileIOManager;
};

/////////////////////////////////////////////////////////////////////////////////////
// The interface to start and end async loads
//
class CPs2AsyncFileLoader : public CAsyncFileLoader
{
public:
	// Opens a "file" by specifying the starting logical sector number of a file
	static CAsyncFileHandle *	sRawOpen( unsigned int lsn, bool blocking = true, int priority = DEFAULT_ASYNC_PRIORITY );

	// Deallocation of buffers cannot be done in interrupt mode.  The FileLoader update function will need to
	// clear them out.
	static void					sAllocateFreeListBuffers( );
	static void					sDeallocateFreeListBuffers( );

	static uint8 *				sGetBuffer( uint size );								// thread only
	static void					sAddBufferToFreeList( uint8 *p_dealloc_buffer );		// thread only
	static void					sIAddBufferToFreeList( uint8 *p_dealloc_buffer );		// interrupt only

private:
	// Constants
	enum
	{
		ALIGN_MAX_BUFFER_SIZE = 17000,
		ALIGN_MIN_BUFFER_SIZE = 4096,
		BUFFER_LIST_SIZE = 2
	};

	// Buffer struct
	struct SAlignBuffer
	{
		uint					m_size;
		volatile bool			m_used;
		volatile uint8 *		mp_buffer;
	};

	static SAlignBuffer			s_buffer_list[BUFFER_LIST_SIZE];
//	static volatile int			s_free_buffer_list_size;
};

/////////////////////////////////////////////////////////////////////////////////////
// File IO manager that communicates with the IOP module
//
class CFileIOManager
{
public:
	static void					sInit();

	// IOP command functions
	static int					sSendCommand(SFileIORequestPacket *p_packet, CPs2AsyncFileHandle *p_file = NULL, bool wait_for_send = false,
											 int continuation_request = -1);
	static void					sWaitSendCompletion(int request_id);
	static void					sWaitRequestCompletion(int request_id);

	static volatile bool		sCommandDone(int request_id);

	static int					sGetRequestResult(int request_id);

private:
	// Constants
	enum
	{
		NUM_REQUESTS = 64
	};

	struct SOpenRequest
	{
		inline volatile bool	CommandDone();

		CPs2AsyncFileHandle *	mp_cur_file;
		int						m_id;
		int						m_sif_cmd_id;
		volatile bool 			m_done;
		volatile int 			m_result;
	};

	static void 				s_result_callback(void *p, void *q);

	static int					s_prev_open_index(int index) { return (index > 0) ? index - 1 : NUM_REQUESTS - 1; }
	static int					s_next_open_index(int index) { return (index + 1) % NUM_REQUESTS; }
	static SOpenRequest *		s_find_request(int request_id);

	// Data for the actual SifCmd transfers
	static sceSifCmdData 		s_cmdbuffer[FILEIO_NUM_COMMAND_HANDLERS];

	static int 					s_cur_request_id;

	// Open requests
	static SOpenRequest			s_open_requests[NUM_REQUESTS];
	static int					s_free_request_index;
};

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline volatile bool		CFileIOManager::SOpenRequest::CommandDone()
{
	return m_done;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace File

#endif  // __SYS_FILE_P_ASYNC_FILESYS_H
