#if 0
#include <sys/replay/replay.h>
#include <sys/ngc/p_aram.h>
#include <sys/ngc/p_dma.h>

#define MAX_BUFFER (20*1024)
#define MAX_FILL_BUFFER (8*1024)

namespace Replay
{

// Must be a multiple of REPLAY_BUFFER_CHUNK_SIZE
#define REPLAY_BUFFER_SIZE 1048576
//#define REPLAY_BUFFER_SIZE ( REPLAY_BUFFER_CHUNK_SIZE * 96 )
//#define REPLAY_BUFFER_SIZE (20480*4)

static uint32 sp_buffer=0;

// Allocates the big buffer for storing replays. Never gets deallocated. (Currently)
void AllocateBuffer()
{
	if (sp_buffer)
	{
		return;
	}	
	
//	sp_buffer=(uint8*)Mem::Malloc(REPLAY_BUFFER_SIZE);
	sp_buffer = NsARAM::alloc( REPLAY_BUFFER_SIZE );
	Dbg_MsgAssert(sp_buffer,("Could not allocate replay buffer"));
}

void DeallocateBuffer()
{
	if (sp_buffer)
	{
		NsARAM::free( sp_buffer );
		sp_buffer=0;
	}
}

uint32 GetBufferSize()
{
	return REPLAY_BUFFER_SIZE;
}

bool BufferAllocated()
{
	if (sp_buffer)
	{
		return true;
	}
	return false;
}
	
// Reads numBytes out of the buffer starting at offset bufferOffset, into p_dest.
// Asserts if there are not enough source bytes.
void ReadFromBuffer(uint8 *p_dest, int bufferOffset, int numBytes)
{
//	OSReport( "READ: 0x%08x - %d bytes\n", bufferOffset, numBytes );
	Dbg_MsgAssert(sp_buffer,("Replay buffer has not been allocated"));
	Dbg_MsgAssert(p_dest,("NULL p_dest sent to Replay::Read()"));
	Dbg_MsgAssert(bufferOffset>=0 && bufferOffset<REPLAY_BUFFER_SIZE,("Bad bufferOffset of %d sent to Read()",bufferOffset));
	Dbg_MsgAssert(bufferOffset+numBytes<=REPLAY_BUFFER_SIZE,("Requested Read() goes past the end of the buffer:\nbufferOffset=%d numBytes=%d REPLAY_BUFFER_SIZE=%d",bufferOffset,numBytes,REPLAY_BUFFER_SIZE));

//	memcpy(p_dest,sp_buffer+bufferOffset,numBytes);

	char buffer[MAX_BUFFER] __attribute__ (( aligned( 32 )));
	int aligned_off = bufferOffset & 0xffffffe0;
	int aligned_size = ( ( ( bufferOffset + numBytes ) - aligned_off ) + 31 ) & 0xffffffe0;

	if ( aligned_size > MAX_BUFFER )
	{
		OSReport( "Transfer too large: %d - max = %d\n", aligned_size, MAX_BUFFER );
		while (1 == 1);
	}

	NsDMA::toMRAM( buffer, sp_buffer+aligned_off, aligned_size );
	memcpy( p_dest, &buffer[bufferOffset-aligned_off], numBytes );
}

// Writes numBytes into the buffer. Asserts if there is not enough space.
void WriteIntoBuffer(uint8 *p_source, int bufferOffset, int numBytes)
{
//	OSReport( "WRITE: 0x%08x - %d bytes\n", bufferOffset, numBytes );

	Dbg_MsgAssert(sp_buffer,("Replay buffer has not been allocated"));
	Dbg_MsgAssert(p_source,("NULL p_source sent to Replay::Write()"));
	Dbg_MsgAssert(bufferOffset>=0 && bufferOffset<REPLAY_BUFFER_SIZE,("Bad bufferOffset of %d sent to Write()",bufferOffset));
	Dbg_MsgAssert(bufferOffset+numBytes<=REPLAY_BUFFER_SIZE,("Requested Write() goes past the end of the buffer:\nbufferOffset=%d numBytes=%d REPLAY_BUFFER_SIZE=%d",bufferOffset,numBytes,REPLAY_BUFFER_SIZE));
	
//	memcpy(sp_buffer+bufferOffset,p_source,numBytes);
	
	// First, we read what's in ARAM to MRAM, then copy our data in and DMA it back.
	char buffer[MAX_BUFFER] __attribute__ (( aligned( 32 )));
	int aligned_off = bufferOffset & 0xffffffe0;
	int aligned_size = ( ( ( bufferOffset + numBytes ) - aligned_off ) + 31 ) & 0xffffffe0;

	if ( aligned_size > MAX_BUFFER )
	{
		OSReport( "Transfer too large: %d - max = %d\n", aligned_size, MAX_BUFFER );
		while (1 == 1);
	}

	NsDMA::toMRAM( buffer, sp_buffer+aligned_off, aligned_size );
	memcpy( &buffer[bufferOffset-aligned_off], p_source, numBytes );
	NsDMA::toARAM( sp_buffer+aligned_off, buffer, aligned_size );
}

void FillBuffer(int bufferOffset, int numBytes, uint8 value)
{
//	OSReport( "FILL: 0x%08x - %d bytes, 0x%02x\n", bufferOffset, numBytes, value );
	Dbg_MsgAssert(sp_buffer,("Replay buffer has not been allocated"));
	Dbg_MsgAssert(bufferOffset>=0 && bufferOffset<REPLAY_BUFFER_SIZE,("Bad bufferOffset of %d sent to FillBuffer()",bufferOffset));
	Dbg_MsgAssert(bufferOffset+numBytes<=REPLAY_BUFFER_SIZE,("Requested FillBuffer() goes past the end of the buffer:\nbufferOffset=%d numBytes=%d REPLAY_BUFFER_SIZE=%d",bufferOffset,numBytes,REPLAY_BUFFER_SIZE));
	
//	memset(sp_buffer+bufferOffset,value,numBytes);
	int off = 0;
	int size = numBytes;
	uint8 buffer[MAX_FILL_BUFFER] __attribute__ (( aligned( 32 )));
	memset(buffer,value,MAX_FILL_BUFFER);

//	if ( ( bufferOffset == 0 ) && ( ( numBytes % MAX_FILL_BUFFER ) == 0 ) )
//	{
//		// Fast version.
//		while ( size > MAX_FILL_BUFFER )
//		{
//			NsDMA::toARAM( sp_buffer+bufferOffset+off, buffer, MAX_FILL_BUFFER );
//			size -= MAX_FILL_BUFFER;
//			off += MAX_FILL_BUFFER;
//		}
//	}
//	else
	{
		while ( size > MAX_FILL_BUFFER )
		{
			WriteIntoBuffer( buffer, bufferOffset+off, MAX_FILL_BUFFER );
			size -= MAX_FILL_BUFFER;
			off += MAX_FILL_BUFFER;
		}
		if ( size ) WriteIntoBuffer( buffer, bufferOffset+off, size ); 
	}

}

}  // namespace Replay

#endif
