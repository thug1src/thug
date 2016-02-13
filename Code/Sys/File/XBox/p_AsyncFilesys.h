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

class CXboxAsyncFileHandle : public CAsyncFileHandle
{
public:

protected:

							CXboxAsyncFileHandle();
	virtual 				~CXboxAsyncFileHandle();

	static VOID CALLBACK	sAsyncFileReadTimerAPCProc( LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue );

private:
	// Constants
	enum
	{
		NUM_REQUESTS = 8
	};

	// Request data
	struct SRequest
	{
		int					m_request_id;
		EAsyncFunctionType	m_function;
	};

	// Callback functions
	virtual void		io_callback(EAsyncFunctionType function, int result, uint32 data);

	// Open requests
	SRequest		  	m_open_requests[NUM_REQUESTS];
	volatile int		m_num_open_requests;

	// Non-aligned buffer IO
	uint8 *				mp_non_aligned_buffer;
	uint8 *				mp_temp_aligned_buffer;

	// Xbox-specific file handle.
	HANDLE				mh_file;
	OVERLAPPED			m_overlapped;	// OVERLAPPED structure required for async reading on Xbox

	bool				add_request_id( int request_id, EAsyncFunctionType function );
	EAsyncFunctionType	get_request_function( int request_id ) const;
	bool				clear_request_id( int request_id );

	// platform-specific calls
	virtual void			plat_init( void );

	virtual bool			plat_open( const char *filename );
	virtual bool			plat_close( void );

	virtual volatile bool	plat_is_done( void );
	virtual volatile bool	plat_is_busy( void );
	virtual bool			plat_is_eof( void ) const;

	virtual void			plat_set_priority( int priority );
	virtual void			plat_set_stream( bool stream );
	virtual void			plat_set_destination( EAsyncMemoryType destination );
	virtual void			plat_set_buffer_size( size_t buffer_size );
	virtual void			plat_set_blocking( bool block );

	virtual size_t			plat_load( void *p_buffer );
	virtual size_t			plat_read( void *p_buffer, size_t size, size_t count );
	virtual size_t			plat_write( void *p_buffer, size_t size, size_t count );
	virtual char *			plat_get_s( char *p_buffer, int maxlen );
	virtual int				plat_seek( long offset, int origin );

	// Friends
	friend CAsyncFileLoader;
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace File

#endif  // __SYS_FILE_P_ASYNC_FILESYS_H
