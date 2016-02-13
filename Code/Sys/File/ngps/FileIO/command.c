/* FileIO -- Converted from Sony samples -- Garrett Oct 2002 */

#include <kernel.h>
#include <libcdvd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <sif.h>
#include <sifrpc.h>
#include <sifcmd.h>
#include <thread.h>
#include "fileio.h"
#include "fileio_iop.h"

// ================================================================

#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

// FileIO prototypes
static int FileIO_GetDescriptor(int memory_destination, int buffer_size);
static int FileIO_FindFileHandleIndex(volatile unsigned int *p_file_handle_index);

static void FileIO_Init(int priority, int buffer_size, short memory_destination, short init_cd_device);
static void FileIO_Open(int sector_num, int file_size, int memory_destination, int buffer_size, int priority, int attributes);
static void FileIO_HostOpen(char *p_filename, int memory_destination, int buffer_size, int priority, int attributes,
							FileIOHostOpen open_type, SFileIORequest *p_cont_request);
static void FileIO_Close(volatile unsigned int *p_file_handle_index);
static void FileIO_Read(SFileIORequest *p_request, volatile unsigned int *p_file_handle_index, void *p_buffer, int size, int non_aligned_start_bytes, int non_aligned_end_bytes);
static void FileIO_Write(SFileIORequest *p_request, volatile unsigned int *p_file_handle_index, void *p_buffer, int size, int non_aligned_start_bytes, int non_aligned_end_bytes);
static void FileIO_Seek(volatile unsigned int *p_file_handle_index, int offset, int origin);
static void FileIO_Set(volatile unsigned int *p_file_handle_index, FileIOVariable variable, int value);

// Debug info
#define SHOW_FILEIO_INFO	0

#if SHOW_FILEIO_INFO
#define ShowFileIOInfo(A...) Kprintf(##A)
#else
#define ShowFileIOInfo(A...) 
#endif

unsigned int gThid = 0;
unsigned int gSem = 0;
unsigned int gCmdSem = 0;

// Defaults
int gDefaultPriority;
int gDefaultBufferSize;
int gDefaultMemoryDestination;

// File handles and buffers
SFileIOHandle	gFileHandle[FILEIO_MAX_HANDLERS];
unsigned char	gFileBuffer[FILEIO_MAX_BUFFERS][FILEIO_BUFFER_SIZE];
unsigned char *	gFileBufferList[FILEIO_MAX_BUFFERS];

int gFreeFileBufferIndex;

// Device status var
static SFileIODevice sFileIODevices[NUM_DEVICE_TYPES];

static SFileIOResultPacket s_result_packet;

static SRequestStatus s_cur_command_status;

static void ServiceCurrentFileHandle(int resend);
static int  StartCdRead(int start_sector, int num_sectors, void *p_buffer);
static void SendNowaitResult();

static int	GetResultPacketSize(SFileIOResultPacket *p_packet);

static void AppendTask(SFileIODevice *p_device, SFileIOTask *p_task);
static void InsertTask(SFileIODevice *p_device, SFileIOTask *p_task);
static void RemoveTask(SFileIODevice *p_device, SFileIOTask *p_task);
static int  AppendContRequest(SFileIOTask *p_task);


//
// Callback
static void cd_callback(int arg)
{
	void *p_ee_addr = NULL, *p_iop_addr = NULL;
	int block_size = 0;
	SFileIODevice *p_device = &sFileIODevices[DEVICE_CD];
	SFileIOHandle *p_file = p_device->mp_file_handle;
	int resend = FALSE;

	// Lame hack to make sure we issued the sceCd call, not some legacy code
	if (!p_device->m_in_use)
	{
		ShowFileIOInfo("Got CD callback even though we didn't issue a command\n");
		return;
	}

	// Wait for previous transfer to finish before we try to read any more (maybe we shouldn't do this in a callback)
	if (p_file->m_flags & FILEIO_HANDLE_PREV_DMA)
	{
		while(sceSifDmaStat(p_file->m_last_dma_id) >= 0)
			;
	}

	// Clear out data transfer size
	s_result_packet.m_non_aligned_data_size = 0;

	switch (p_file->m_cur_function)
	{
	case FILEIO_FUNCTION_READ:
		// Check that we got the correct arg back.  Re-send command if not.
		if ((arg != SCECdFuncRead) && (arg != SCECdFuncSeek))
		{
			resend = TRUE;
			break;
		}

		// We probably won't set this for an IOP memory operation
		s_result_packet.m_byte_transfer = p_file->m_bytes_processing;

		// Figure out if we are done before calling ServiceCurrentFileHandle()
		s_result_packet.m_done = (p_file->m_bytes_to_process == 0);
		s_result_packet.m_header.opt = p_file->m_request_id;
		s_result_packet.m_return_value = p_file->m_return_value;
		s_result_packet.m_return_data = p_file->m_return_data;

		// Do we need to transfer to EE?
		if (p_file->m_flags & FILEIO_HANDLE_EE_MEMORY)
		{
			p_iop_addr = p_device->mp_buffer[p_device->m_buffer_index];
			p_device->m_buffer_index ^= 1;

			p_ee_addr = p_file->mp_dest_buffer;
			block_size = p_file->m_bytes_processing;

			// Check non-alignment
			if (p_file->m_non_aligned_start_bytes)
			{
				s_result_packet.m_non_aligned_data_size = p_file->m_non_aligned_start_bytes;
				memcpy(s_result_packet.m_non_aligned_data, p_iop_addr, p_file->m_non_aligned_start_bytes);

				p_file->m_non_aligned_start_bytes = 0;
				//if (s_result_packet.m_done && p_file->m_non_aligned_end_bytes)
				//{
				//	Kprintf("Have both non-aligned data at beginning and end of buffer\n");
				//}
			}
			else if (s_result_packet.m_done && p_file->m_non_aligned_end_bytes)
			{
				s_result_packet.m_non_aligned_data_size = p_file->m_non_aligned_end_bytes;
				memcpy(s_result_packet.m_non_aligned_data, p_iop_addr + (block_size - p_file->m_non_aligned_end_bytes), p_file->m_non_aligned_end_bytes);
			}
		}

		// Advance dest pointer
		p_file->mp_dest_buffer = p_file->mp_dest_buffer + block_size;

		break;

	case FILEIO_FUNCTION_OPEN:
	case FILEIO_FUNCTION_SEEK:
		s_result_packet.m_byte_transfer = 0;
		s_result_packet.m_done = TRUE;
		s_result_packet.m_return_value = p_file->m_return_value;
		s_result_packet.m_return_data = p_file->m_return_data;
		s_result_packet.m_header.opt = s_cur_command_status.m_request_id;
		p_file->m_cur_function = FILEIO_FUNCTION_IDLE;
		break;

	default:
		// Something went wrong here
		ShowFileIOInfo("Got cd callback from an unexpected function: %d\n", p_file->m_cur_function);
		s_result_packet.m_byte_transfer = 0;
		s_result_packet.m_done = TRUE;
		s_result_packet.m_return_value = -1;
		s_result_packet.m_return_data = -1;
		s_result_packet.m_header.opt = s_cur_command_status.m_request_id;
		break;

	}

	if (!resend)
	{
		p_file->m_flags |= FILEIO_HANDLE_PREV_DMA;
		p_file->m_last_dma_id = isceSifSendCmd(FILEIO_RESULT_COMMAND, &s_result_packet, GetResultPacketSize(&s_result_packet), p_iop_addr, p_ee_addr, block_size);
	}
	ShowFileIOInfo("FileIO cd callback: sent result %d back from %x to %x of size %d with data %x %x %x %x and arg %x error %d resend %d\n", s_result_packet.m_return_value, p_iop_addr, p_ee_addr, block_size,
				   ((unsigned int *) p_iop_addr)[0],
				   ((unsigned int *) p_iop_addr)[1],
				   ((unsigned int *) p_iop_addr)[2],
				   ((unsigned int *) p_iop_addr)[3],
				   arg, sceCdGetError(), resend );

	// Check to see if we need to send out more CD commands
	ServiceCurrentFileHandle(resend);

	// Wake up the dispatch thread
	iSignalSema(gCmdSem);
}

// Can be called from both from outside and within a CD callback
static void ServiceCurrentFileHandle(int resend)
{
	int sectors_to_read;
	SFileIODevice *p_device = &sFileIODevices[DEVICE_CD];
	SFileIOHandle *p_file = p_device->mp_file_handle;

	// Read parameters: need to save in case of resend
	static int cur_sector;
	static int num_sectors;
	static void *p_local_buffer;

	if (resend)
	{
		ShowFileIOInfo("ServiceCurrentFileHandle %x: Resending read of %d sectors starting at %d to %x with %d bytes left\n", p_file, num_sectors, cur_sector, p_local_buffer, p_file->m_bytes_to_process);

		StartCdRead(cur_sector, num_sectors, p_local_buffer);

		return;
	}

	// Right now, this only does something with reads
	if (p_file->m_cur_function != FILEIO_FUNCTION_READ)
	{
		// We're done
		p_file->m_flags &= ~FILEIO_HANDLE_BUSY;
		p_device->m_in_use = FALSE;

		return;
	}

	sectors_to_read = (p_file->m_bytes_to_process + SECTOR_SIZE - 1) / SECTOR_SIZE;

	// Check if we need to seek first
	if (p_file->m_cur_sector != p_device->m_cur_sector)
	{
		p_device->m_cur_sector = p_file->m_cur_sector;
		p_device->m_cur_position = p_file->m_cur_position;

		// We don't need to seek since the read does it for us.
		//sceCdSeek(p_file->m_cur_sector);
		//ShowFileIOInfo("ServiceCurrentFileHandle %x: Seeking to sector %d\n", p_file, p_file->m_cur_sector);
		//return;
	}

	if (sectors_to_read)
	{
		cur_sector = p_file->m_cur_sector;

		if (p_file->m_flags & FILEIO_HANDLE_EE_MEMORY)
		{
			p_local_buffer = p_device->mp_buffer[p_device->m_buffer_index];
			num_sectors = MIN(FILEIO_BUFFER_SECTORS, sectors_to_read);
		}
		else
		{
			// Note that we don't handle the last sector properly, which most likely won't be
			// filled (and would overwrite the output buffer).
			p_local_buffer = p_file->mp_dest_buffer;
			num_sectors = sectors_to_read;
		}

		// Advance the file stats before the read call, since, in theory, the read could call the
		// callback immediately
		p_file->m_cur_sector += num_sectors;
		p_file->m_cur_position += num_sectors * SECTOR_SIZE;
		p_file->m_bytes_processing = MIN(num_sectors * SECTOR_SIZE, p_file->m_bytes_to_process);
		p_file->m_bytes_to_process = MAX(p_file->m_bytes_to_process - p_file->m_bytes_processing, 0);

		ShowFileIOInfo("ServiceCurrentFileHandle %x: Reading %d sectors starting at %d to %x with %d bytes left\n", p_file, num_sectors, cur_sector, p_local_buffer, p_file->m_bytes_to_process);
		p_device->m_cur_sector = p_file->m_cur_sector;
		p_device->m_cur_position = p_file->m_cur_position;

		StartCdRead(cur_sector, num_sectors, p_local_buffer);
	} else {
		// We're done
		p_file->m_flags &= ~FILEIO_HANDLE_BUSY;
		p_device->m_in_use = FALSE;

		// Send message to EE that we are done
	}
}

#define NUM_WAIT_CLICKS 1000

static int StartCdRead(int start_sector, int num_sectors, void *p_buffer)
{
	int result;
	sceCdRMode mode;
	int retries = 0;
	static int wait_read = 0;

	if (wait_read)
	{
		ShowFileIOInfo("Got callback while waiting to resend\n");
		return 0;
	}

	wait_read = 1;

	mode.trycount = 255;
	mode.spindlctrl = SCECdSpinNom;
	mode.datapattern = SCECdSecS2048;
	mode.pad = 0;

	do
	{
		//if (sceCdSync( 1 ))
		//	ShowFileIOInfo("Sync isn't ready before the CD read\n");

		result = sceCdRead(start_sector, num_sectors, p_buffer, &mode);

		if (result == 0)
		{
			int i, j = 0;

			Kprintf("sceCdRead() didn't start.  Returned %d with error %d.  Retrying...\n", result, sceCdGetError());

			sceCdSync( 0 );

			do
			{
				for ( i = 0; i < NUM_WAIT_CLICKS; i++ )
					;
				if ( j++ > NUM_WAIT_CLICKS )
				{
					j = 0;
				}
			} while ( j != 0 );

            while(sceCdDiskReady(0)!=SCECdComplete)
				;
			//sceCdSeek(0);
			//sceCdSync( 0 );
			//result = 1;
			retries++;
		}
	} while ((result == 0) && (retries <= 15));		// Need to eventually abort this if retries get too high

	wait_read = 0;

	return result;
}

// Returns packet size of result, rounded up to a 16-byte chunk
static int GetResultPacketSize(SFileIOResultPacket *p_packet)
{
	unsigned int base_size = ((unsigned int) &(p_packet->m_non_aligned_data[0]) - (unsigned int) p_packet);

	return ((base_size + p_packet->m_non_aligned_data_size) + 0xf) & ~0xf;
}

////////////////////////////////
// Tries to find the real file handle index from the open request ID
static int FileIO_FindFileHandleIndex(volatile unsigned int *p_file_handle_index)
{
	int fd;
	int open_request_id = (int) (*p_file_handle_index & ~FILEIO_TEMP_HANDLE_FLAG);

	for (fd = 0; fd < FILEIO_MAX_HANDLERS; fd++)
	{
		if (gFileHandle[fd].m_open_request_id == open_request_id)
		{
			ShowFileIOInfo("FileIO: converting open request id %d to file handle index %d\n", open_request_id, fd);
			*p_file_handle_index = fd;
			return TRUE;
		}
	}

	ShowFileIOInfo("FileIO: can't convert open request id %d to file handle index\n", open_request_id);
	return FALSE;
}

////////////////////////////////
// Init
static void FileIO_Init(int priority, int buffer_size, short memory_destination, short init_cd_device)
{
	int detected_disk_type;
	int i;

	printf("Starting FileIO_Init\n");

	sFileIODevices[DEVICE_CD].m_in_use = FALSE;
	sFileIODevices[DEVICE_CD].mp_file_handle = NULL;

	// Init drive
	if (init_cd_device)
	{
#if 0		// Doesn't seem to be necessary, since we already do this on the EE
		sceCdInit(SCECdINIT);
		sceCdMmode(SCECdDVD);

		// find the disk type, this might be different from what we intialized to 
		printf("FileIO: sceCdGetDiskType   ");
		detected_disk_type= sceCdGetDiskType();
		switch(detected_disk_type)
		{
			case SCECdIllgalMedia:
			printf("Disk Type= IllgalMedia\n"); break;
			case SCECdPS2DVD:
			printf("Disk Type= PlayStation2 DVD\n"); break;
			case SCECdPS2CD:
			printf("Disk Type= PlayStation2 CD\n"); break;
			case SCECdPS2CDDA:
			printf("Disk Type= PlayStation2 CD with CDDA\n"); break;
			case SCECdPSCD:
			printf("Disk Type= PlayStation CD\n"); break;
			case SCECdPSCDDA:
			printf("Disk Type= PlayStation CD with CDDA\n"); break;
			case SCECdDVDV:
			printf("Disk Type= DVD video\n"); break;
			case SCECdCDDA:
			printf("Disk Type= CD-DA\n"); break;
			case SCECdDETCT:
			printf("Working\n"); break;
			case SCECdNODISC: 
			printf("Disk Type= No Disc\n"); break;
			default:
			printf("Disk Type= OTHER DISK\n"); break;
		}

		// If disk type has changed to a CD, then need to re-initialize it							   
		if (detected_disk_type == SCECdPS2CD)
		{
			#if __DVD_ONLY__
			printf( "*** ERROR - CD Detected, needs DVD.\n" );
			#else
			printf( "reinitializing for Ps2CD\n" );
			sceCdMmode(SCECdCD);
			printf( "done reinitializing\n" );
			#endif		
		}
#endif

		// Set up callback
		sceCdCallback(cd_callback);
	}

	// Set default parameters
	gDefaultPriority			= priority;
	gDefaultBufferSize			= buffer_size;
	gDefaultMemoryDestination	= memory_destination;

	// Init the file handles and buffers
	for (i = 0; i < FILEIO_MAX_HANDLERS; i++)
	{
		gFileHandle[i].m_flags			= 0;		// Neither busy nor reserved
		//gFileHandle[i].mp_buffer[0]		= NULL;
		//gFileHandle[i].mp_buffer[1]		= NULL;
		//gFileHandle[i].m_buffer_index  	= -1;
		gFileHandle[i].mp_dest_buffer	= NULL;
		gFileHandle[i].m_device_type	= DEVICE_UNKNOWN;
		gFileHandle[i].m_buffer_size	= buffer_size;
		gFileHandle[i].m_host_fd		= -1;
		gFileHandle[i].m_priority		= priority;
		gFileHandle[i].m_stream			= FALSE;
		gFileHandle[i].m_start_sector	= 0;
		gFileHandle[i].m_cur_sector		= 0;
		gFileHandle[i].m_cur_position	= 0;
		gFileHandle[i].m_file_size		= 0;
		gFileHandle[i].m_open_request_id= -1;
	}

	for (i = 0; i < FILEIO_MAX_BUFFERS; i++)
	{
		gFileBufferList[i] = gFileBuffer[i];
	}

	gFreeFileBufferIndex = 0;

	// Allocate device buffers
	for (i = 0; i < DEVICE_UNKNOWN; i++)
	{
		if ((gFreeFileBufferIndex + 1) < FILEIO_MAX_BUFFERS)
		{
			// Use buffer size in the future
			sFileIODevices[i].mp_buffer[0] = gFileBufferList[gFreeFileBufferIndex++];
			sFileIODevices[i].mp_buffer[1] = gFileBufferList[gFreeFileBufferIndex++];
			sFileIODevices[i].m_buffer_index = 0;
		}
		else
		{
			// Can't allocate
			printf("Out of fileio memory buffers\n");
			s_cur_command_status.m_request_state = REQUEST_COMPLETE;
			s_cur_command_status.m_return_value = -1;
			return;
		}
	}

	printf("Done FileIO_Init\n");

	s_cur_command_status.m_request_state = REQUEST_COMPLETE;
	s_cur_command_status.m_return_value = TRUE;
	return;
}

////////////////////////////////
// GetDescriptor
static int FileIO_GetDescriptor(int memory_destination, int buffer_size)
{
	int i;

	for (i = 0; i < FILEIO_MAX_HANDLERS; i++)
	{
		if (!(gFileHandle[i].m_flags & FILEIO_HANDLE_RESERVED))
		{
			gFileHandle[i].m_buffer_size = buffer_size;

			if (memory_destination == MEM_EE)
			{
				gFileHandle[i].m_flags |= FILEIO_HANDLE_EE_MEMORY;
#if 0
				if ((gFreeFileBufferIndex + 1) < FILEIO_MAX_BUFFERS)
				{
					// Use buffer size in the future
					gFileHandle[i].mp_buffer[0] = gFileBufferList[gFreeFileBufferIndex++];
					gFileHandle[i].mp_buffer[1] = gFileBufferList[gFreeFileBufferIndex++];
					gFileHandle[i].m_buffer_index = 0;
				}
				else
				{
					// Can't open
					printf("Out of fileio memory buffers\n");
					return -1;
				}
#endif
			}
			else
			{
				gFileHandle[i].m_flags &= ~FILEIO_HANDLE_EE_MEMORY;
#if 0
				gFileHandle[i].mp_buffer[0] = NULL;
				gFileHandle[i].mp_buffer[1] = NULL;
				gFileHandle[i].m_buffer_index = -1;
#endif
			}

			gFileHandle[i].m_flags |= FILEIO_HANDLE_RESERVED;

			return i;
		}
	}

	// Couldn't find a free descriptor
	printf("Out of fileio file descriptors\n");
	return -1;

}

////////////////////////////////
// Open
static void FileIO_Open(int sector_num, int file_size, int memory_destination, int buffer_size, int priority, int attributes)
{
	int fd = FileIO_GetDescriptor(memory_destination, buffer_size);

	if (fd >= 0)
	{
		// Init the handle itself
		gFileHandle[fd].m_device_type = DEVICE_CD;
		gFileHandle[fd].mp_dest_buffer = NULL;
		gFileHandle[fd].m_host_fd = -1;
		gFileHandle[fd].m_priority = priority;
		gFileHandle[fd].m_stream = FALSE;
		gFileHandle[fd].m_start_sector = sector_num;
		gFileHandle[fd].m_cur_sector = sector_num;
		gFileHandle[fd].m_cur_position = 0;
		gFileHandle[fd].m_file_size = file_size;
		gFileHandle[fd].m_open_request_id= s_cur_command_status.m_request_id;
		gFileHandle[fd].mp_blocked_request = NULL;
		gFileHandle[fd].m_flags &= ~FILEIO_HANDLE_BUSY;
		gFileHandle[fd].m_cur_function = FILEIO_FUNCTION_IDLE;

		// Don't do seek, just return
		gFileHandle[fd].m_return_value = fd;
		gFileHandle[fd].m_return_data = file_size;
		s_cur_command_status.m_request_state = REQUEST_COMPLETE;
		s_cur_command_status.mp_file_handle = &(gFileHandle[fd]);
		s_cur_command_status.m_return_value = fd;
		s_cur_command_status.m_return_data = file_size;
		ShowFileIOInfo("Opened file using handle %d\n", fd);
	} else {
		s_cur_command_status.m_request_state = REQUEST_COMPLETE;
		s_cur_command_status.m_return_value = -1;
		return;
	}
}

////////////////////////////////
// HostOpen
static void FileIO_HostOpen(char *p_filename, int memory_destination, int buffer_size, int priority, int attributes,
							FileIOHostOpen open_type, SFileIORequest *p_cont_request)
{
	int fd, host_fd;
	char full_filename[FILEIO_MAX_FILENAME_SIZE * 2];

	strcpy(full_filename, p_filename);
	if (open_type == FILEIO_HOSTOPEN_EXTEND_NAME)
	{
		if (p_cont_request)
		{
			strcat(full_filename, p_cont_request->m_param.m_open.m_file.m_filename);
			ShowFileIOInfo("FileIO HostOpen: opening extended filename %s\n", full_filename);
		}
		else
		{
			ShowFileIOInfo("FileIO HostOpen: waiting for rest of filename for %s\n", p_filename);
			s_cur_command_status.m_request_state = REQUEST_OPEN;
			return;
		}
	}

	host_fd = open(full_filename, O_RDONLY);
	if (host_fd >= 0)
	{
		// Find file size
		int file_size = lseek(host_fd, 0, SEEK_END);
		lseek(host_fd, 0, SEEK_SET);

		fd = FileIO_GetDescriptor(memory_destination, buffer_size);

		if (fd >= 0)
		{
			// Init the handle itself
			gFileHandle[fd].m_device_type = DEVICE_HOST;
			gFileHandle[fd].mp_dest_buffer = NULL;
			gFileHandle[fd].m_priority = priority;
			gFileHandle[fd].m_stream = FALSE;
			gFileHandle[fd].m_start_sector = -1;
			gFileHandle[fd].m_cur_sector = -1;
			gFileHandle[fd].m_cur_position = 0;
			gFileHandle[fd].m_file_size = file_size;
			gFileHandle[fd].m_open_request_id= s_cur_command_status.m_request_id;
			gFileHandle[fd].mp_blocked_request = NULL;
			gFileHandle[fd].m_flags &= ~FILEIO_HANDLE_BUSY;
			gFileHandle[fd].m_cur_function = FILEIO_FUNCTION_IDLE;

			// Do a open
			gFileHandle[fd].m_host_fd = host_fd;

			gFileHandle[fd].m_return_value = fd;
			gFileHandle[fd].m_return_data = file_size;

			s_cur_command_status.m_request_state = REQUEST_COMPLETE;
			s_cur_command_status.mp_file_handle = &(gFileHandle[fd]);
			s_cur_command_status.m_return_value = fd;
			s_cur_command_status.m_return_data = file_size;
			return;
		} else {
			printf("Can't get free descriptor for host open of %s\n", full_filename);
			close(host_fd);
		}
	} else {
		printf("Can't open file %s on host\n", full_filename);
	}

	s_cur_command_status.m_request_state = REQUEST_COMPLETE;
	s_cur_command_status.m_return_value = -1;
	return;
}

////////////////////////////////
// Close
static void FileIO_Close(volatile unsigned int *p_file_handle_index)
{
	SFileIOHandle *p_file;
	//int i, j;

	// See if we have the correct index, and if we can't get it, return
	if ((*p_file_handle_index & FILEIO_TEMP_HANDLE_FLAG) && !FileIO_FindFileHandleIndex(p_file_handle_index))
	{
		s_cur_command_status.m_request_state = REQUEST_OPEN;
		return;
	}

	p_file = &(gFileHandle[*p_file_handle_index]);

	if (p_file->m_flags & FILEIO_HANDLE_RESERVED)
	{
		s_cur_command_status.mp_file_handle = p_file;

		// Can't close if we're busy
		if (p_file->m_flags & FILEIO_HANDLE_BUSY)
		{
			s_cur_command_status.m_request_state = REQUEST_OPEN;
			return;
		}

		// Check if a host handle
		if (p_file->m_host_fd >= 0)
		{
			close(p_file->m_host_fd);
		}

#if 0
		// Check if buffer needs freeing
		if (p_file->mp_buffer[0])
		{
			for (i = 0; i < FILEIO_MAX_BUFFERS; i++)
			{
				for (j = 0; j < 2; j++)
				{
					if (p_file->mp_buffer[j] == gFileBufferList[i])
					{
						// Found it, exchange it with last used buffer
						gFileBufferList[i] = gFileBufferList[--gFreeFileBufferIndex];
						gFileBufferList[gFreeFileBufferIndex] = p_file->mp_buffer[j];
						p_file->mp_buffer[j] = NULL;
					}
				}
			}
			p_file->m_buffer_index = -1;
		}
#endif

		p_file->m_flags &= ~FILEIO_HANDLE_RESERVED;

		ShowFileIOInfo("CLOSE: handle %d open and not busy\n", *p_file_handle_index);
		s_cur_command_status.m_request_state = REQUEST_COMPLETE;
		s_cur_command_status.m_return_value = TRUE;
		return;
	}

	// Handle wasn't open
	ShowFileIOInfo("CLOSE: handle %d not open\n", *p_file_handle_index);
	s_cur_command_status.m_request_state = REQUEST_COMPLETE;
	s_cur_command_status.m_return_value = FALSE;
	return;
}

////////////////////////////////
// Read
static void FileIO_Read(SFileIORequest *p_request, volatile unsigned int *p_file_handle_index, void *p_buffer, int size, int non_aligned_start_bytes, int non_aligned_end_bytes)
{
	SFileIOHandle *p_file;
	int host_read;
	int bytes_to_read;

	// See if we have the correct index, and if we can't get it, return
	if ((*p_file_handle_index & FILEIO_TEMP_HANDLE_FLAG) && !FileIO_FindFileHandleIndex(p_file_handle_index))
	{
		s_cur_command_status.m_request_state = REQUEST_OPEN;
		return;
	}

	p_file = &(gFileHandle[*p_file_handle_index]);
	host_read = (p_file->m_host_fd >= 0);

	s_cur_command_status.mp_file_handle = p_file;

	// Can't add additional read if we're already busy
	if ((p_file->m_flags & FILEIO_HANDLE_BUSY) && !(p_file->mp_blocked_request && (p_file->mp_blocked_request == p_request)))
	{
		s_cur_command_status.m_request_state = REQUEST_OPEN;
		return;
	}

	// Check to make sure device is free
	if (host_read)
	{
		// Nothing to check for now
	}
	else
	{
		if (sFileIODevices[DEVICE_CD].m_in_use)
		{
			// Mark file handle as blocked
			p_file->mp_blocked_request = p_request;
			p_file->m_flags |= FILEIO_HANDLE_BUSY;

			s_cur_command_status.m_request_state = REQUEST_OPEN;
			return;
		}
		else
		{
			// Clear blocked status
			p_file->mp_blocked_request = NULL;
			p_file->m_flags &= ~FILEIO_HANDLE_BUSY;
		}
	}

	// We won't support non sector aligned reads now
	if (!host_read && (p_file->m_cur_position & (SECTOR_SIZE - 1)))
	{
		printf("Can't read from position %d because it isn't on a sector boundary\n", p_file->m_cur_position);
		p_file->m_cur_function = FILEIO_FUNCTION_IDLE;
		s_cur_command_status.m_request_state = REQUEST_COMPLETE;
		s_cur_command_status.m_return_value = -1;
		return;
	}

	p_file->m_bytes_to_process = MIN(size, p_file->m_file_size - p_file->m_cur_position);
	p_file->m_bytes_processing = 0;
	p_file->m_non_aligned_start_bytes = non_aligned_start_bytes;
	p_file->m_non_aligned_end_bytes = non_aligned_end_bytes;
	p_file->m_request_id = s_cur_command_status.m_request_id;

	// Set destination buffer (EE or IOP address) and size
	p_file->mp_dest_buffer = p_buffer;
	bytes_to_read = p_file->m_bytes_to_process;

	// Do the actual read
	if (host_read)
	{
		SFileIODevice *p_device = &sFileIODevices[DEVICE_HOST];

		// We should really be looking for the old one and making sure it is done
		p_file->m_flags &= ~FILEIO_HANDLE_PREV_DMA;

		// Just do the read here until we set up a separate thread
		while (p_file->m_bytes_to_process)
		{
			void *p_ee_addr = NULL, *p_iop_addr = NULL;
			int block_size = 0;
			int num_bytes;
			void *p_local_buffer;

			if (p_file->m_flags & FILEIO_HANDLE_EE_MEMORY)
			{
				p_local_buffer = p_device->mp_buffer[p_device->m_buffer_index];
				num_bytes = MIN(FILEIO_BUFFER_SIZE, p_file->m_bytes_to_process);
			}
			else
			{
				p_local_buffer = p_file->mp_dest_buffer;
				num_bytes = p_file->m_bytes_to_process;
			}

			// Advance the file stats before the read call, since, in theory, the read could call the
			// callback immediately
			p_file->m_cur_position += num_bytes;
			p_file->m_bytes_processing = num_bytes;
			p_file->m_bytes_to_process = p_file->m_bytes_to_process - p_file->m_bytes_processing;

			ShowFileIOInfo("Host read %x: Reading %d bytes to %x with %d bytes left\n", p_file, num_bytes, p_local_buffer, p_file->m_bytes_to_process);
			read(p_file->m_host_fd, p_local_buffer, num_bytes);

			// Wait for previous transfer to finish before we try to read any more (maybe we shouldn't do this in a callback)
			if (p_file->m_flags & FILEIO_HANDLE_PREV_DMA)
			{
				while(sceSifDmaStat(p_file->m_last_dma_id) >= 0)
					;
			}

			// We probably won't set this for an IOP memory operation
			s_result_packet.m_byte_transfer = p_file->m_bytes_processing;

			// Figure out if we are done before calling ServiceCurrentFileHandle()
			s_result_packet.m_done = (p_file->m_bytes_to_process == 0);
			s_result_packet.m_header.opt = p_file->m_request_id;
			s_result_packet.m_return_value = bytes_to_read;
			s_result_packet.m_non_aligned_data_size = 0;

			// Do we need to transfer to EE?
			if (p_file->m_flags & FILEIO_HANDLE_EE_MEMORY)
			{
				p_iop_addr = p_local_buffer;
				p_device->m_buffer_index ^= 1;

				p_ee_addr = p_file->mp_dest_buffer;
				block_size = p_file->m_bytes_processing;

				// Check non-alignment
				if (p_file->m_non_aligned_start_bytes)
				{
					s_result_packet.m_non_aligned_data_size = p_file->m_non_aligned_start_bytes;
					memcpy(s_result_packet.m_non_aligned_data, p_iop_addr, p_file->m_non_aligned_start_bytes);

					p_file->m_non_aligned_start_bytes = 0;
				}
				else if (s_result_packet.m_done && p_file->m_non_aligned_end_bytes)
				{
					s_result_packet.m_non_aligned_data_size = p_file->m_non_aligned_end_bytes;
					memcpy(s_result_packet.m_non_aligned_data, p_iop_addr + (block_size - p_file->m_non_aligned_end_bytes), p_file->m_non_aligned_end_bytes);
				}
			}

			// Advance dest pointer
			p_file->mp_dest_buffer = p_file->mp_dest_buffer + block_size;

			p_file->m_flags |= FILEIO_HANDLE_PREV_DMA;
			p_file->m_last_dma_id = sceSifSendCmd(FILEIO_RESULT_COMMAND, &s_result_packet, GetResultPacketSize(&s_result_packet), p_iop_addr, p_ee_addr, block_size);
			ShowFileIOInfo("FileIO host read: sent result %d back from %x to %x of size %d\n", s_result_packet.m_return_value, p_iop_addr, p_ee_addr, block_size);
		}

		s_cur_command_status.m_request_state = REQUEST_PROCESSING;		// Don't want to send another result packet
		p_file->m_return_value = bytes_to_read;
	}
	else
	{
		// We should really be looking for the old one and making sure it is done
		p_file->m_flags &= ~FILEIO_HANDLE_PREV_DMA;

		// Mark that we are using the CD
		p_file->m_flags |= FILEIO_HANDLE_BUSY;
		sFileIODevices[DEVICE_CD].m_in_use = TRUE;
		sFileIODevices[DEVICE_CD].mp_file_handle = p_file;

		p_file->m_cur_function = FILEIO_FUNCTION_READ;
		p_file->m_return_value = p_file->m_bytes_to_process;
		s_cur_command_status.m_request_state = REQUEST_PROCESSING;

		ServiceCurrentFileHandle(FALSE);
	}

	return;
}

////////////////////////////////
// Write
static void FileIO_Write(SFileIORequest *p_request, volatile unsigned int *p_file_handle_index, void *p_buffer, int size, int non_aligned_start_bytes, int non_aligned_end_bytes)
{
	SFileIOHandle *p_file;

	// See if we have the correct index, and if we can't get it, return
	if ((*p_file_handle_index & FILEIO_TEMP_HANDLE_FLAG) && !FileIO_FindFileHandleIndex(p_file_handle_index))
	{
		s_cur_command_status.m_request_state = REQUEST_OPEN;
		return;
	}

	p_file = &(gFileHandle[*p_file_handle_index]);

	s_cur_command_status.mp_file_handle = p_file;

	// Can't write if we're already busy
	if ((p_file->m_flags & FILEIO_HANDLE_BUSY) && !(p_file->mp_blocked_request && (p_file->mp_blocked_request == p_request)))
	{
		s_cur_command_status.m_request_state = REQUEST_OPEN;
		return;
	}

	s_cur_command_status.m_request_state = REQUEST_COMPLETE;
	s_cur_command_status.m_return_value = -1;
	return;
}

////////////////////////////////
// Seek
static void FileIO_Seek(volatile unsigned int *p_file_handle_index, int offset, int origin)
{
	int new_position;
	SFileIOHandle *p_file;

	// See if we have the correct index, and if we can't get it, return
	if ((*p_file_handle_index & FILEIO_TEMP_HANDLE_FLAG) && !FileIO_FindFileHandleIndex(p_file_handle_index))
	{
		s_cur_command_status.m_request_state = REQUEST_OPEN;
		return;
	}

	p_file = &(gFileHandle[*p_file_handle_index]);

	s_cur_command_status.mp_file_handle = p_file;

	// Can't start seek if we're already busy
	if (p_file->m_flags & FILEIO_HANDLE_BUSY)
	{
		s_cur_command_status.m_request_state = REQUEST_OPEN;
		return;
	}

	// Do the adjustments
	switch (origin)
	{
	case SEEK_CUR:
		new_position = p_file->m_cur_position + offset;
		break;

	case SEEK_SET:
		new_position = offset;
		break;

	case SEEK_END:
		new_position = p_file->m_file_size - offset;
		break;

	default:
		p_file->m_cur_function = FILEIO_FUNCTION_IDLE;
		s_cur_command_status.m_request_state = REQUEST_COMPLETE;
		s_cur_command_status.m_return_value = -1;
		return;
	}

	// Check if before beginning
	if (new_position < 0)
	{
		new_position = 0;
	}

	// Check if past EOF
	if (new_position > p_file->m_file_size)
	{
		new_position = p_file->m_file_size;
	}

	if (p_file->m_host_fd >= 0)
	{
		// Device ready
		p_file->m_cur_position = new_position;

		// Do the seek
		lseek(p_file->m_host_fd, p_file->m_cur_position, SEEK_SET);

		p_file->m_return_value = p_file->m_cur_position;
		s_cur_command_status.m_request_state = REQUEST_COMPLETE;
		s_cur_command_status.m_return_value = p_file->m_cur_position;
	}
	else
	{
		// Change position
		p_file->m_cur_position = new_position;

		// Also set the new current sector
		p_file->m_cur_sector = p_file->m_start_sector + (p_file->m_cur_position / SECTOR_SIZE);

		p_file->m_return_value = p_file->m_cur_position;
		s_cur_command_status.m_request_state = REQUEST_COMPLETE;
		s_cur_command_status.m_return_value = p_file->m_cur_position;
	}

	return;
}

static void FileIO_Set(volatile unsigned int *p_file_handle_index, FileIOVariable variable, int value)
{
	SFileIOHandle *p_file;

	// See if we have the correct index, and if we can't get it, return
	if ((*p_file_handle_index & FILEIO_TEMP_HANDLE_FLAG) && !FileIO_FindFileHandleIndex(p_file_handle_index))
	{
		s_cur_command_status.m_request_state = REQUEST_OPEN;
		return;
	}

	p_file = &(gFileHandle[*p_file_handle_index]);

	s_cur_command_status.mp_file_handle = p_file;

	// Can't change parameters if we're already busy
	if (p_file->m_flags & FILEIO_HANDLE_BUSY)
	{
		s_cur_command_status.m_request_state = REQUEST_OPEN;
		return;
	}

	s_cur_command_status.m_request_state = REQUEST_COMPLETE;

	switch (variable)
	{
	case FILEIO_VAR_PRIORITY:
		p_file->m_priority = value;
		break;

	case FILEIO_VAR_BUFFER_SIZE:
		p_file->m_buffer_size = value;
		break;

	case FILEIO_VAR_STREAM:
		p_file->m_stream = value;
		break;

	case FILEIO_VAR_DESTINATION:
		// Not sure how to handle this one yet, since buffers may need to be acquired or freed
		if (value == MEM_EE)
		{
			p_file->m_flags |= FILEIO_HANDLE_EE_MEMORY;
		}
		else
		{
			p_file->m_flags &= ~FILEIO_HANDLE_EE_MEMORY;
		}
		break;

	default:
		s_cur_command_status.m_return_value = FALSE;
		return;
	}

	s_cur_command_status.m_return_value = TRUE;
	return;
}

// EzADPCM_INIT
int AdpcmInit( int pIopBuf )
{
#if 0
	int i;
	if ( gEECommand & PCMSTATUS_INITIALIZED )
	{
		return ( 0 );
	}

    if ( gSem == 0 )
	{
		struct SemaParam sem;
		sem.initCount = 0;
		sem.maxCount = 1;
		sem.attr = AT_THFIFO;
		gSem = CreateSema (&sem);
    }
    if (gThid == 0)
	{
		struct ThreadParam param;
		param.attr         = TH_C;
		param.entry        = RefreshStreams;
		param.initPriority = BASE_priority-3;
		param.stackSize    = 0x800; // was 800
		param.option = 0;
		gThid = CreateThread (&param);
		printf( "EzADPCM: create thread ID= %d\n", gThid );
		StartThread (gThid, (u_long) NULL);
    }

	memset( gStreamInfo, 0, sizeof( struct StreamInfo ) * NUM_STREAMS );
	memset( &gMusicInfo, 0, sizeof( struct StreamInfo ) );
	
	gpIopBuf = pIopBuf;
	gEECommand |= PCMSTATUS_INITIALIZED;
	//printf( "PCM Irx iop buf %d\n", gpIopBuf );
	//Dbug_Printf( "PCM irx is initialized\n" );
#endif
    return gThid;
}

// EzADPCM_QUIT
void AdpcmQuit( void )
{
    DeleteThread (gThid);
    gThid = 0;
#if 0
    DeleteSema (gSem);
    gSem = 0;
#endif
    return;
}

/* internal */
int RefreshStreams( int status )
{
	//int i;

    while ( 1 )
	{
		WaitSema(gSem);
#if 0
		for ( i = 0; i < NUM_STREAMS; i++ )
		{
			CheckStream( i );
		}
		CheckMusic( );

		for ( i = 0; i < NUM_STREAMS; i++ )
		{
			while ( gStreamInfo[ i ].update )
			{
				gStreamInfo[ i ].update--;
				UpdateStream( i );
			}
		}
		while ( gMusicInfo.update )
		{
			gMusicInfo.update--;
			UpdateMusic( );
		}
		if ( gMusicInfo.volumeSet )
		{
			gMusicInfo.volumeSet = 0;
			AdpcmSetMusicVolumeDirect( gMusicInfo.volume );
		}
		for ( i = 0; i < NUM_STREAMS; i++ )
		{
			if ( gStreamInfo[ i ].volumeSet )
			{
				gStreamInfo[ i ].volumeSet = 0;
				AdpcmSetStreamVolumeDirect( gStreamInfo[ i ].volume, i );
			}
		}
#endif
    }
    return 0;
}

static ERequestState dispatch(volatile SFileIOTask *p_request_task)
{ 
	volatile SFileIORequestEntry *p_request_entry = &(p_request_task->m_entry);
	volatile SFileIORequest *p_request = &(p_request_entry->m_request);
	SFileIORequest *p_cont_request = (p_request_task->mp_cont) ? (&p_request_task->mp_cont->m_entry.m_request) : NULL;

	// Assume command will need to be completed asynchronously
	s_cur_command_status.m_request_state = REQUEST_OPEN;
	s_cur_command_status.m_request_id = p_request_entry->m_request_id;
	s_cur_command_status.mp_file_handle = NULL;

    switch ( p_request->m_command )
	{
	case FILEIO_INIT:
		FileIO_Init(p_request->m_param.m_init.m_priority,
					p_request->m_param.m_init.m_buffer_size,
					p_request->m_param.m_init.m_memory_dest,
					p_request->m_param.m_init.m_init_cd_device);
		break;
		
	case FILEIO_OPEN:
		if (p_request->m_param.m_open.m_host_read)
		{
			FileIO_HostOpen((char *) p_request->m_param.m_open.m_file.m_filename,
							p_request->m_param.m_open.m_memory_dest,
							p_request->m_param.m_open.m_buffer_size,
							p_request->m_param.m_open.m_priority,
							p_request->m_param.m_open.m_attributes,
							p_request->m_param.m_open.m_host_read,
							p_cont_request);
		} else {
			FileIO_Open(p_request->m_param.m_open.m_file.m_raw_name.m_sector_number, 
						p_request->m_param.m_open.m_file.m_raw_name.m_file_size,
						p_request->m_param.m_open.m_memory_dest,
						p_request->m_param.m_open.m_buffer_size,
						p_request->m_param.m_open.m_priority,
						p_request->m_param.m_open.m_attributes);
		}
		break;

	case FILEIO_CLOSE:
		FileIO_Close(&p_request->m_param.m_close.m_file_handle);
		break;

	case FILEIO_READ:
		FileIO_Read((SFileIORequest *) p_request, &p_request->m_param.m_rw.m_file_handle, p_request->m_param.m_rw.m_buffer,
					p_request->m_param.m_rw.m_size, p_request->m_param.m_rw.m_non_aligned_start_bytes,
					p_request->m_param.m_rw.m_non_aligned_end_bytes);
		break;

	case FILEIO_WRITE:
		FileIO_Write((SFileIORequest *) p_request, &p_request->m_param.m_rw.m_file_handle, p_request->m_param.m_rw.m_buffer,
					 p_request->m_param.m_rw.m_size, p_request->m_param.m_rw.m_non_aligned_start_bytes,
					 p_request->m_param.m_rw.m_non_aligned_end_bytes);
		break;

	case FILEIO_SEEK:
		FileIO_Seek(&p_request->m_param.m_seek.m_file_handle, p_request->m_param.m_seek.m_offset,
					p_request->m_param.m_seek.m_origin);
		break;

	case FILEIO_SET:
		FileIO_Set(&p_request->m_param.m_set.m_file_handle, p_request->m_param.m_set.m_variable,
				   p_request->m_param.m_set.m_value);
		break;

	case FILEIO_CONT:
		s_cur_command_status.m_request_state = REQUEST_APPEND;
		break;

	default:
		ERROR ("FileIO driver error: unknown command %d \n", p_request->m_command);
		s_cur_command_status.m_return_value = -1;
		break;
    }

	// Copy file handle in case we need it later
	p_request_task->mp_file_handle = s_cur_command_status.mp_file_handle;

	switch (s_cur_command_status.m_request_state)
	{
	case REQUEST_OPEN:
		// This isn't an error or warning anymore: it's now normal
		//ERROR (("FileIO driver error: request %d should not end in a open request state\n", p_request_entry->m_request_id));
		//printf("FileIO driver warning: request %d with command %d ended in a open request state\n", p_request_entry->m_request_id, p_request->m_command);
		break;

	case REQUEST_PROCESSING:
		break;

	case REQUEST_APPEND:
		break;

	case REQUEST_COMPLETE:
		// Send result back to EE now if command completed
		SendNowaitResult();
		break;
	}

    return s_cur_command_status.m_request_state;
}


static sceSifCmdData cmdbuffer[FILEIO_NUM_COMMAND_HANDLERS];

#define NUM_FILEIO_REQUESTS	(16)

// Note these can be changed in an interrupt
static volatile SFileIORequestEntry sFileIORequestArray[NUM_FILEIO_REQUESTS];
static volatile int sFirstFileIORequest;		// Interrupt only reads this value.
static volatile	int sFreeFileIORequest;			// This is the main variable that changes in the interrupt

// The task entries
static SFileIOTask sFileIOTaskArray[FILEIO_MAX_TASKS];
static SFileIOTask *spTaskFreeList = NULL;

static void request_callback(void *p, void *q);

int fileio_command_loop( void )
{
	int oldStat;
	int i;
	EDeviceType cur_device;

	// Create semaphore to signal command came in
	struct SemaParam sem;
	sem.initCount = 0;
	sem.maxCount = 1;
	sem.attr = AT_THFIFO;
	gCmdSem = CreateSema(&sem);

	// Init the stream queue
	sFirstFileIORequest = 0;
	sFreeFileIORequest = 0;

	// Init free task list
	spTaskFreeList = NULL;
	for (i = 0; i < FILEIO_MAX_TASKS; i++)
	{
		sFileIOTaskArray[i].mp_file_handle = NULL;

		// Add to beginning of list
		sFileIOTaskArray[i].mp_next = spTaskFreeList;
		spTaskFreeList = &sFileIOTaskArray[i];
	}

	// Init devices
	for (i = 0; i < NUM_DEVICE_TYPES; i++)
	{
		sFileIODevices[i].m_in_use = FALSE;
		sFileIODevices[i].m_waiting_callback = FALSE;
		sFileIODevices[i].mp_file_handle = NULL;
		sFileIODevices[i].mp_task_list = NULL;
		sFileIODevices[i].m_cur_position = -1;
		sFileIODevices[i].m_cur_sector = -1;
		sFileIODevices[i].mp_buffer[0] = NULL;
		sFileIODevices[i].mp_buffer[1] = NULL;
		sFileIODevices[i].m_buffer_index = -1;
	}

	sceSifInitRpc(0);

	// set local buffer & functions
	CpuSuspendIntr(&oldStat);

	// SIFCMD
	sceSifSetCmdBuffer( &cmdbuffer[0], FILEIO_NUM_COMMAND_HANDLERS);

	sceSifAddCmdHandler(FILEIO_REQUEST_COMMAND, (void *) request_callback, NULL );

	CpuResumeIntr(oldStat);

	// The loop
	while (1) {
		//ShowFileIOInfo("waiting for command semaphore\n");
		WaitSema(gCmdSem);		// Get signal from callback
		//ShowFileIOInfo("got command semaphore\n");

		// Move new requests into schedule list
		// Note that FreeStreamRequest can change in the interrupt at any time
		// Also, FirstStreamRequest is examined in the interrupt, but just to make sure we don't overflow the buffer
		while (sFreeFileIORequest != sFirstFileIORequest)
		{
			volatile SFileIORequestEntry *p_request = &(sFileIORequestArray[sFirstFileIORequest]);
			SFileIOTask *p_new_task;
			ShowFileIOInfo("FileIO: got request id %d with command %x\n", p_request->m_request_id, p_request->m_request.m_command);

			// Get a new task entry
			p_new_task = spTaskFreeList;
			spTaskFreeList = p_new_task->mp_next;
			p_new_task->mp_next = NULL;

			// Copy data
			p_new_task->m_entry = *p_request;
			p_new_task->mp_file_handle = NULL;
			p_new_task->mp_cont = NULL;

			// And put onto unknown device task list
			AppendTask(&sFileIODevices[DEVICE_UNKNOWN], p_new_task);

			// Increment request index; Note that interrupt can look at this
			sFirstFileIORequest = (sFirstFileIORequest + 1) % NUM_FILEIO_REQUESTS;
		}

		// Process device tasks
		for (cur_device = DEVICE_CD; cur_device < NUM_DEVICE_TYPES; cur_device++)
		{
			SFileIODevice *p_device = &sFileIODevices[cur_device];
			SFileIOTask *p_task;

			// Skip if busy
			if (p_device->m_in_use)
			{
				continue;
			}

			p_task = p_device->mp_task_list;
			while (p_task)
			{
				ERequestState ret;
				SFileIOTask *p_dispatch_task;

				p_dispatch_task = p_task;

				ShowFileIOInfo("FileIO: doing request id %d with command %x\n", p_dispatch_task->m_entry.m_request_id, p_dispatch_task->m_entry.m_request.m_command);
				ret = dispatch(p_dispatch_task);

				p_task = p_task->mp_next;

				switch (ret)
				{
				case REQUEST_OPEN:
					// Move to other device
					if (p_dispatch_task->mp_file_handle && (p_dispatch_task->mp_file_handle->m_device_type != cur_device))
					{
						RemoveTask(p_device, p_dispatch_task);
						InsertTask(&sFileIODevices[p_dispatch_task->mp_file_handle->m_device_type], p_dispatch_task);
					}
					break;

				case REQUEST_APPEND:
					// Add the request to an existing request
					if (AppendContRequest(p_dispatch_task))
					{
						ShowFileIOInfo("***************** FileIO: appended request id %d of command %x\n", p_dispatch_task->m_entry.m_request_id, p_dispatch_task->m_entry.m_request.m_command);
						RemoveTask(p_device, p_dispatch_task);
						SignalSema(gCmdSem);		// So the other task can be retried
					}
					break;

				case REQUEST_PROCESSING:
				case REQUEST_COMPLETE:
					RemoveTask(p_device, p_dispatch_task);
					if (p_dispatch_task->mp_cont)			// Check if we have a continuation request
					{
						p_dispatch_task->mp_cont->mp_next = spTaskFreeList;
						spTaskFreeList = p_dispatch_task->mp_cont;
						p_dispatch_task->mp_cont = NULL;
					}
					p_dispatch_task->mp_next = spTaskFreeList;
					spTaskFreeList = p_dispatch_task;

					break;
				}
			}
		}
	}

    return 0;
}

static void SendNowaitResult()
{
	static SFileIOResultPacket fileIOResult;

	static unsigned int last_cmd_id = 0;
	static int did_dma_send = FALSE;

	// Send the result back
	if (did_dma_send && last_cmd_id >= 0)	// Wait for previous send to complete (it should already be done, though)
	{
	   while(sceSifDmaStat(last_cmd_id) >= 0)
		   Kprintf("Waiting for DMA\n");
	}

	// Gotta copy the id into SStreamRequest
	fileIOResult.m_header.opt = s_cur_command_status.m_request_id;	// Copy id
	fileIOResult.m_return_value = s_cur_command_status.m_return_value;
	fileIOResult.m_return_data = s_cur_command_status.m_return_data;
	fileIOResult.m_done = TRUE;
	fileIOResult.m_non_aligned_data_size = 0;
	did_dma_send = TRUE;
	last_cmd_id = sceSifSendCmd(FILEIO_RESULT_COMMAND, &fileIOResult, GetResultPacketSize(&fileIOResult), 0, 0, 0);
	ShowFileIOInfo("FileIO: sent nowait result id %d back\n", s_cur_command_status.m_request_id);
}

static void request_callback(void *p, void *q)
{
	SFileIORequestPacket *h = (SFileIORequestPacket *) p;

	// Check to make sure we can add
	int nextFreeReq = (sFreeFileIORequest + 1) % NUM_FILEIO_REQUESTS;
	if (nextFreeReq == sFirstFileIORequest)
	{
		// We can't allow a request to be ignored.  We must abort
		ERROR(("******************************** FileIO driver error: too many requests ********************************\n" ));
	}

	ShowFileIOInfo("FileIO: received request id %d with command %x\n", h->m_header.opt, h->m_request.m_command);

	// Copy the request into the reuest queue
	sFileIORequestArray[sFreeFileIORequest].m_request = h->m_request;
	sFileIORequestArray[sFreeFileIORequest].m_request_id = h->m_header.opt;
	sFreeFileIORequest = nextFreeReq;

	// And wake up the dispatch thread
	iSignalSema(gCmdSem);
}

// Adds p_task to the end of the device task list
static void AppendTask(SFileIODevice *p_device, SFileIOTask *p_task)
{
	SFileIOTask *p_task_list = p_device->mp_task_list;
	if (p_task_list)
	{
		// Find end of list
		while (p_task_list->mp_next)
		{
			p_task_list = p_task_list->mp_next;
		}

		p_task->mp_next = p_task_list->mp_next;
		p_task_list->mp_next = p_task;
	}
	else
	{
		// Empty list
		p_device->mp_task_list = p_task;
		p_task->mp_next = NULL;
	}
}

#define GetPriority(x) ((x) ? (x)->m_priority : gDefaultPriority)

// Inserts p_task into the device task list sorted by priority
static void InsertTask(SFileIODevice *p_device, SFileIOTask *p_task)
{
	SFileIOTask *p_task_node = p_device->mp_task_list;
	SFileIOTask *p_prev_node = NULL;

	int priority = GetPriority(p_task->mp_file_handle);

	// Find point in list
	while (p_task_node)
	{
		if (priority < GetPriority(p_task_node->mp_file_handle))
		{
			break;
		}
		p_prev_node = p_task_node;
		p_task_node = p_task_node->mp_next;
	}

	// Insert
	if (p_prev_node)
	{
		p_task->mp_next = p_task_node;
		p_prev_node->mp_next = p_task;
	}
	else
	{
		// Head of list
		p_task->mp_next = p_device->mp_task_list;
		p_device->mp_task_list = p_task;
	}
}

// Removes p_task from the device task list
static void RemoveTask(SFileIODevice *p_device, SFileIOTask *p_task)
{
	SFileIOTask *p_task_node = p_device->mp_task_list;

	if (p_task == p_task_node)
	{
		// Head of list
		p_device->mp_task_list = p_task->mp_next;
	}
	else
	{
		// Find item in list
		while (p_task_node->mp_next)
		{
			if (p_task == p_task_node->mp_next)
			{
				break;
			}
			p_task_node = p_task_node->mp_next;
		}

		// Remove
		p_task_node->mp_next = p_task->mp_next;
	}
	p_task->mp_next = NULL;
}

static int AppendContRequest(SFileIOTask *p_task)
{
	EDeviceType cur_device;

	// Process device tasks
	for (cur_device = DEVICE_CD; cur_device < NUM_DEVICE_TYPES; cur_device++)
	{
		SFileIODevice *p_device = &sFileIODevices[cur_device];
		SFileIOTask *p_task_node;

		p_task_node = p_device->mp_task_list;

		// Find item in list
		while (p_task_node)
		{
			if (p_task->m_entry.m_request_id == p_task_node->m_entry.m_request_id && (p_task != p_task_node))
			{
				p_task_node->mp_cont = p_task;
				return TRUE;
			}

			p_task_node = p_task_node->mp_next;
		}
	}

	return FALSE;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
