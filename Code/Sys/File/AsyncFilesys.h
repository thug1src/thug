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
**	Created:		09/24/02	-	grj										**
**																			**
**	File name:		core/sys/AsyncFilesys.h									**
**																			**
*****************************************************************************/

#ifndef	__SYS_FILE_ASYNC_FILESYS_H
#define	__SYS_FILE_ASYNC_FILESYS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __GEL_MODULE_H
#include <gel/module.h>
#endif

#include <sys/file/AsyncTypes.h>
#include <sys/file/Pre.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define   	DEFAULT_ASYNC_PRIORITY	(100)
#define   	DEFAULT_BUFFER_SIZE		(64 * 1024)

namespace Pcm
{
	class CFileStreamInfo;
}

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
class CAsyncFileHandle;
class CAsyncFileLoader;
class CAsyncFilePoll;

/////////////////////////////////////////////////////////////////////////////////////
// The asynchronous file class
//
class CAsyncFileHandle
{
public:

	// A callback is called after the completion of every async function
	typedef void (* AsyncCallback)(CAsyncFileHandle *, EAsyncFunctionType function, int result, unsigned int arg0, unsigned int arg1);


	// Check read status
	volatile bool		IsDone( void );
	volatile bool		IsBusy( void );
	bool				IsEOF( void ) const;
	int					WaitForIO( void );

	// Change parameters
	void				SetPriority( int priority );
	void				SetStream( bool stream );			// Tells if loaded all at once or streamed one buffer at a time
	void				SetDestination( EAsyncMemoryType destination );
	void				SetBufferSize( size_t buffer_size );
	void				SetBlocking( bool block );
	void				SetCallback( AsyncCallback p_callback, unsigned int arg0, unsigned int arg1 );

	size_t				GetFileSize( void );

	// Async commands
	size_t				Load( void *p_buffer );
	size_t				Read( void *p_buffer, size_t size, size_t count );
	size_t				Write( void *p_buffer, size_t size, size_t count );
	char *				GetS( char *p_buffer, int maxlen );	// Probably don't need this one
	int					Seek( long offset, int origin );

protected:
						CAsyncFileHandle();
	virtual 			~CAsyncFileHandle();

	void				init( void );

	bool				open( const char *filename, bool blocking = true, int priority = DEFAULT_ASYNC_PRIORITY );
	bool				close( void );

	// Internal busy counter functions
	volatile int		inc_busy_count();			// Increment and return current count
	volatile int		dec_busy_count();			// Decrement and return current count
	volatile int		get_busy_count();			// Just return current count

	// Internal callback
	virtual void		io_callback(EAsyncFunctionType function, int result, uint32 data);
	void				post_io_callback();

	// These could be changed by an interrupt, so they are volatile
	volatile int		m_last_result;

	// Callback
	AsyncCallback		mp_callback;
	unsigned int		m_callback_arg0;
	unsigned int		m_callback_arg1;
	EAsyncFunctionType	m_current_function;		// So callback knows what was completed

	EAsyncMemoryType	m_mem_destination;
	bool				m_stream;
	bool				m_blocking;
	uint32				m_buffer_size;
	int					m_priority;

	volatile int		m_file_size;
	volatile int		m_position;

	// PRE file
	PreFile::FileHandle *mp_pre_file;

	// constants
	static const uint32	MAX_FILE_SIZE;

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

private:
	volatile int		m_busy_count;		// Number of items we are waiting to finish asynchronously

	// Friends
	friend CAsyncFileLoader;

};

/////////////////////////////////////////////////////////////////////////////////////
// The interface to start and end async loads
//
class CAsyncFileLoader
{
public:
	static void					sInit();
	static void					sCleanup();

	static bool					sAsyncSupported();							// Returns true if async has been implemented on
																			// the platform

	/////////////////////////////////////////////////////////////////////
	// The following are our versions of the ANSI file IO functions.
	//
	static bool					sExist( const char *filename );				// Nothing special about this function
	static CAsyncFileHandle *	sOpen( const char *filename, bool blocking = true, int priority = DEFAULT_ASYNC_PRIORITY );
	static bool					sClose( CAsyncFileHandle *p_file_handle );

	// IO Waiting
	static void					sWaitForIOEvent(bool all_io_events);		// Returns when we get an async result
	static bool					sAsyncInUse();								// Slow


protected:
	// Constants
	enum
	{
		MAX_FILE_HANDLES = 		16,
		MAX_PENDING_CALLBACKS = 16,
	};

	// Callback data
	struct SCallback
	{
		CAsyncFileHandle *				mp_file_handle;
		EAsyncFunctionType				m_function;
		int								m_result;
	};

	// This should actually be declared below the p-line
	static CAsyncFileHandle *	s_file_handles[MAX_FILE_HANDLES];
	static int					s_free_handle_index;

	// Status
	static volatile int			s_manager_busy_count;		// Is 0 when there are no outstanding async IO requests
	static volatile bool		s_new_io_completion;		// File handle got callback since s_execute_callback_list()

	// Current callback list
	static volatile SCallback	s_callback_list[2][MAX_PENDING_CALLBACKS];
	static volatile int			s_num_callbacks[2];
	static int					s_cur_callback_list_index;

	// File handle management
	static CAsyncFileHandle *	s_get_file_handle();
	static bool					s_free_file_handle(CAsyncFileHandle *p_file_handle);

	// callback list functions
	static void					s_add_callback(CAsyncFileHandle *p_file, EAsyncFunctionType function, int result);
	static void					s_execute_callback_list();

	// Increment and decrement the manager busy count.  Should only be called by CAsyncFileHandle::inc_busy_count()
	// and CAsyncFileHandle::dec_busy_count()
	static void					s_inc_manager_busy_count();
	static void					s_dec_manager_busy_count();

	// Primarily to allow per-frame polling for platforms that may not support system callbacks on i/o routines
	static void					s_update();

	// platform-specific calls
	static void					s_plat_init();
	static void					s_plat_cleanup();

	static bool					s_plat_async_supported();

	static bool					s_plat_exist( const char *filename );
	static CAsyncFileHandle *	s_plat_open( const char *filename, int priority );
	static bool					s_plat_close( CAsyncFileHandle *p_file_handle );

	static void					s_plat_swap_callback_list();		// This is needed below the p-line because interrupts
																	// need to be disabled for a bit
	static void					s_plat_update();

	// Friends
	friend CAsyncFilePoll;
	friend CAsyncFileHandle;
};

/////////////////////////////////////////////////////////////////////////////////////
// This defines the singleton class for the one-a-frame async calls that are needed.
// For now, just the callbacks that have stacked up in CAsyncFileLoader are called
//
class CAsyncFilePoll  : public Mdl::Module
{
	DeclareSingletonClass( CAsyncFilePoll );

			CAsyncFilePoll();
	virtual ~CAsyncFilePoll();

	void v_start_cb ( void );
	void v_stop_cb ( void );
	
	static Tsk::Task< CAsyncFilePoll >::Code s_logic_code;       
	Tsk::Task< CAsyncFilePoll > *mp_logic_task;
};

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

inline void				CAsyncFileLoader::s_inc_manager_busy_count()
{
	s_manager_busy_count++;
}

inline void				CAsyncFileLoader::s_dec_manager_busy_count()
{
	s_manager_busy_count--;
}

inline volatile int		CAsyncFileHandle::inc_busy_count()
{
	CAsyncFileLoader::s_inc_manager_busy_count();			// Also increment manager
	return ++m_busy_count;
}

inline volatile int		CAsyncFileHandle::dec_busy_count()
{
	CAsyncFileLoader::s_dec_manager_busy_count();			// Also decrement manager
	return --m_busy_count;
}

inline volatile int		CAsyncFileHandle::get_busy_count()
{
	return m_busy_count;
}

} // namespace File

#endif  // __SYS_FILE_ASYNC_FILESYS_H
