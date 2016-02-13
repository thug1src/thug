// This file is included by both the EE and IOP side.  Make sure all
// changes are C compatible.

#ifndef	__SYS_FILE_NGPS_FILEIO_H
#define	__SYS_FILE_NGPS_FILEIO_H

#include <sifcmd.h>

#ifndef SECTOR_SIZE
#define SECTOR_SIZE							( 2048 )
#endif // SECTOR_SIZE

// Make sure these don't conflict with the Stream IRX
#define FILEIO_REQUEST_COMMAND				(1)
#define FILEIO_RESULT_COMMAND				(1)
#define FILEIO_NUM_COMMAND_HANDLERS 		(0x10)

#define FILEIO_TEMP_HANDLE_FLAG				(0x80000000)

#define FILEIO_MAX_FILENAME_SIZE			(80)

#define FILEIO_BUFFER_SECTORS				(8)
#define FILEIO_BUFFER_SIZE					(FILEIO_BUFFER_SECTORS * SECTOR_SIZE)

#define FILEIO_CACHE_BLOCK_SIZE				(64)								// Cache block size for EE
#define FILEIO_CACHE_BLOCK_NONALIGN_MASK	(FILEIO_CACHE_BLOCK_SIZE - 1)		// Masks out the non-aligned address bits
#define FILEIO_CACHE_BLOCK_ALIGN_MASK		(~FILEIO_CACHE_BLOCK_NONALIGN_MASK)	// Masks out the aligned address bits

// The FileIO commands
typedef enum
{
	FILEIO_INIT = 0,
	FILEIO_OPEN,
	FILEIO_CLOSE,
	FILEIO_READ,
	FILEIO_WRITE,
	FILEIO_SEEK,
	FILEIO_SET,
	FILEIO_CONT,
} FileIOCommand;

typedef enum
{
	FILEIO_VAR_PRIORITY,
	FILEIO_VAR_BUFFER_SIZE,
	FILEIO_VAR_STREAM,
	FILEIO_VAR_DESTINATION,
} FileIOVariable;

typedef enum
{
	FILEIO_HOSTOPEN_FALSE = 0,
	FILEIO_HOSTOPEN_TRUE,
	FILEIO_HOSTOPEN_EXTEND_NAME,
} FileIOHostOpen;

////////////////////////////////////
// Command parameters
// All structures sizes + 4 bytes (for command itself) must be 16-byte aligned
//

// General Parameters
typedef struct
{
	int				m_param[3];
} SGeneralParam;

// Init Parameters
typedef struct
{
	int				m_priority;
	int				m_buffer_size;
	short			m_memory_dest;
	short			m_init_cd_device;
} SInitParam;

// Open Parameters
typedef struct
{
	int				m_sector_number;
	int				m_file_size;
} SRawFileName;

typedef struct
{
	int				m_buffer_size;
	int				m_priority;

	// Unions must be named in C
	union
	{
		char		 m_filename[FILEIO_MAX_FILENAME_SIZE];
		SRawFileName m_raw_name;
	} m_file;

	short			m_attributes;
	unsigned char	m_host_read;
	unsigned char	m_memory_dest;
} SOpenParam;

// Close Parameters
typedef struct
{
	unsigned int	m_file_handle;
	int				m_pad[2];
} SCloseParam;

// Read/Write Parameters
typedef struct
{
	unsigned int	m_file_handle;
	void *			m_buffer;
	size_t			m_size;
	unsigned short	m_non_aligned_start_bytes;
	unsigned short	m_non_aligned_end_bytes;
	int				m_pad[3];
} SRWParam;

// Seek Parameters
typedef struct
{
	unsigned int	m_file_handle;
	int				m_offset;
	int				m_origin;
} SSeekParam;

// Set Parameters
typedef struct
{
	unsigned int	m_file_handle;
	FileIOVariable	m_variable;
	int				m_value;
} SSetParam;

// Parameter Union
typedef union
{
	SGeneralParam	m_general;
	SInitParam		m_init;
	SOpenParam		m_open;
	SCloseParam		m_close;
	SRWParam		m_rw;
	SSeekParam		m_seek;
	SSetParam		m_set;
} SFileIOParam;

// Make sure this doesn't break the alignment of SSifCmdFileIOReqPacket
typedef struct
{
	// Add union first so it aligns correctly
	SFileIOParam		m_param;
	FileIOCommand		m_command;
} SFileIORequest;


////////////////////////////////////
// Other structures
// 

// SifCmd structures ( keeping them 128-bit aligned )
typedef struct
{
	sceSifCmdHdr	m_header;
	SFileIORequest	m_request;
} SFileIORequestPacket;

// Packet that is sent back to EE after a request is made
typedef struct
{
	sceSifCmdHdr	m_header;
	int				m_return_value;			// Return value
	int				m_done;					// Indicates request completed
	int				m_byte_transfer;		// Number of bytes copied in secondary DMA
	int				m_return_data;			// Additional return data

	// Non-aligned data buffer (for beginning and end of buffers that don't cover a whole cache block)
	int				m_non_aligned_data_size;
	unsigned char	m_non_aligned_data[FILEIO_CACHE_BLOCK_SIZE + 12];	// Need the additional 12 bytes to align this packet
} SFileIOResultPacket;

// Stores the request
typedef struct FileIORequestEntry
{
	SFileIORequest				m_request;
	int							m_request_id;
} SFileIORequestEntry;

#endif	// __SYS_FILE_NGPS_FILEIO_H

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
