
// This file is included by the IOP side only.  Make sure all
// changes are C compatible.

#ifndef	__SYS_FILE_NGPS_FILEIO_IOP_H
#define	__SYS_FILE_NGPS_FILEIO_IOP_H

#include "fileio.h"


#if 0
#define PRINTF(x) printf x
#else
#define PRINTF(x) 
#endif
//#define ERROR(x) printf x
#define ERROR(A...) Kprintf(##A); while(1)
#define xPRINTF(x) 

#define BASE_priority  32

#define TRUE 1
#define FALSE 0

#define FILEIO_MAX_HANDLERS		(16)
#define FILEIO_MAX_BUFFERS		((NUM_DEVICE_TYPES - 1) * 2)		// Don't need buffer for the Unknown device
#define FILEIO_MAX_TASKS		(20)

// Function enums (for callback)
typedef enum
{
	FILEIO_FUNCTION_IDLE = 0,					// No function is being waited on
	FILEIO_FUNCTION_OPEN,
	FILEIO_FUNCTION_SEEK,
	FILEIO_FUNCTION_READ,
	FILEIO_FUNCTION_WRITE,
} EFileIOFunction;

// Device types
typedef enum
{
	DEVICE_CD = 0,
	DEVICE_HOST,
	DEVICE_UNKNOWN,		// This is always the last device type (also where all requests start)
	NUM_DEVICE_TYPES,
} EDeviceType;

typedef enum
{
	MEM_EE = 0,
	MEM_IOP,
} EMemoryType;

// FileIOHandle Flags
#define	FILEIO_HANDLE_BUSY		(0x0001)		// Handle currently busy
#define	FILEIO_HANDLE_RESERVED	(0x0002)		// Handle currently allocated to someone
#define	FILEIO_HANDLE_PREV_DMA	(0x0004)		// Already sent out DMA info (meaning m_last_dma_id is valid)
#define	FILEIO_HANDLE_NOWAIT	(0x0008)		// Last function called finished immediately
#define	FILEIO_HANDLE_EE_MEMORY	(0x0010)		// Destionion is EE memory

// File handle
typedef struct
{
	// General data
	volatile int	m_flags;
//	void *			mp_buffer[2];		// This is only needed for EE transfers
//	int				m_buffer_index;		// This is only needed for EE transfers
	void *			mp_dest_buffer;		// Could be either an EE or IOP address
	int				m_buffer_size;
	EDeviceType		m_device_type;
	int				m_host_fd;			// File descriptor for host IO
	int				m_priority;
	int				m_stream;
	int				m_start_sector;
	int				m_cur_sector;
	int				m_cur_position;
	int				m_file_size;
	int				m_open_request_id;
	SFileIORequest *mp_blocked_request;	// If handle is marked BUSY but waiting for the device, this is the request that
										// will execute when the device finally becomes free.

	// Current IO variables
	EFileIOFunction	m_cur_function;
	int				m_bytes_to_process;
	int				m_bytes_processing;
	unsigned short	m_non_aligned_start_bytes;
	unsigned short	m_non_aligned_end_bytes;
	int				m_request_id;
	int				m_return_value;
	int				m_return_data;
	unsigned int	m_last_dma_id;
} SFileIOHandle;

// Request task
typedef struct FileIOTask
{
	SFileIORequestEntry 	m_entry;
	SFileIOHandle *			mp_file_handle;
	struct FileIOTask *		mp_cont;	// Continued packets for the same request
	struct FileIOTask *		mp_next;
} SFileIOTask;

// Device status
typedef struct
{
	volatile int	m_in_use;
	volatile int	m_waiting_callback;
	SFileIOHandle *	mp_file_handle;
	void *			mp_buffer[2];		// This is only needed for EE transfers
	int				m_buffer_index;		// This is only needed for EE transfers
	int				m_cur_sector;
	int				m_cur_position;
	SFileIOTask *	mp_task_list;
} SFileIODevice;


// Request state
typedef enum
{
	REQUEST_OPEN,			// Still waiting to be started
	REQUEST_PROCESSING,		// Currently executing
	REQUEST_APPEND,			// Needs appending to a current request
	REQUEST_COMPLETE,		// Done
} ERequestState;

// Request Status
typedef struct
{
	int				m_request_id;
	ERequestState	m_request_state;
	SFileIOHandle *	mp_file_handle;
	int				m_return_value;
	int				m_return_data;
} SRequestStatus;

#endif	// __SYS_FILE_NGPS_FILEIO_IOP_H

