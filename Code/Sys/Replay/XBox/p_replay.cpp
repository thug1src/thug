#include <sys/replay/replay.h>

#if 0

namespace Replay
{

// Must be a multiple of REPLAY_BUFFER_CHUNK_SIZE
#define REPLAY_BUFFER_SIZE 1048576

static uint8 *sp_buffer=NULL;

// Allocates the big buffer for storing replays. Never gets deallocated. (Currently)
void AllocateBuffer()
{
	if (sp_buffer)
	{
		return;
	}	
	
	sp_buffer=(uint8*)Mem::Malloc(REPLAY_BUFFER_SIZE);
	Dbg_MsgAssert(sp_buffer,("Could not allocate replay buffer"));
}

void DeallocateBuffer()
{
	if (sp_buffer)
	{
		Mem::Free(sp_buffer);
		sp_buffer=NULL;
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
	Dbg_MsgAssert(sp_buffer,("Replay buffer has not been allocated"));
	Dbg_MsgAssert(p_dest,("NULL p_dest sent to Replay::Read()"));
	Dbg_MsgAssert(bufferOffset>=0 && bufferOffset<REPLAY_BUFFER_SIZE,("Bad bufferOffset of %d sent to Read()",bufferOffset));
	Dbg_MsgAssert(bufferOffset+numBytes<=REPLAY_BUFFER_SIZE,("Requested Read() goes past the end of the buffer:\nbufferOffset=%d numBytes=%d REPLAY_BUFFER_SIZE=%d",bufferOffset,numBytes,REPLAY_BUFFER_SIZE));

	memcpy(p_dest,sp_buffer+bufferOffset,numBytes);
}

// Writes numBytes into the buffer. Asserts if there is not enough space.
void WriteIntoBuffer(uint8 *p_source, int bufferOffset, int numBytes)
{
	Dbg_MsgAssert(sp_buffer,("Replay buffer has not been allocated"));
	Dbg_MsgAssert(p_source,("NULL p_source sent to Replay::Write()"));
	Dbg_MsgAssert(bufferOffset>=0 && bufferOffset<REPLAY_BUFFER_SIZE,("Bad bufferOffset of %d sent to Write()",bufferOffset));
	Dbg_MsgAssert(bufferOffset+numBytes<=REPLAY_BUFFER_SIZE,("Requested Write() goes past the end of the buffer:\nbufferOffset=%d numBytes=%d REPLAY_BUFFER_SIZE=%d",bufferOffset,numBytes,REPLAY_BUFFER_SIZE));
	
	memcpy(sp_buffer+bufferOffset,p_source,numBytes);
}

void FillBuffer(int bufferOffset, int numBytes, uint8 value)
{
	Dbg_MsgAssert(sp_buffer,("Replay buffer has not been allocated"));
	Dbg_MsgAssert(bufferOffset>=0 && bufferOffset<REPLAY_BUFFER_SIZE,("Bad bufferOffset of %d sent to FillBuffer()",bufferOffset));
	Dbg_MsgAssert(bufferOffset+numBytes<=REPLAY_BUFFER_SIZE,("Requested FillBuffer() goes past the end of the buffer:\nbufferOffset=%d numBytes=%d REPLAY_BUFFER_SIZE=%d",bufferOffset,numBytes,REPLAY_BUFFER_SIZE));
	
	memset(sp_buffer+bufferOffset,value,numBytes);
}

}  // namespace Replay

#endif
