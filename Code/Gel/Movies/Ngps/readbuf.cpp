#include "gel/movies/ngps/defs.h"
#include "gel/movies/ngps/readbuf.h"

#define BUFF_SIZE (N_READ_UNIT * READ_UNIT_SIZE)

namespace Flx
{



// ////////////////////////////////////////////////////////////////
//
// Create read buffer
//
void readBufCreate(ReadBuf *b)
{
    b->put = b->count = 0;
    b->size = BUFF_SIZE;
}

// ////////////////////////////////////////////////////////////////
//
// Delete read buffer
//
void readBufDelete(ReadBuf *b)
{
}

// ////////////////////////////////////////////////////////////////
//
// Get empty area
//
int readBufBeginPut(ReadBuf *b, u_char **ptr)
{
    int size = b->size - b->count;
    if (size) {
        *ptr = b->data + b->put;
    }
    return size;
}

// ////////////////////////////////////////////////////////////////
//
// Proceed 'write' pointer
//
int readBufEndPut(ReadBuf *b, int size)
{
    int size_ok = min(b->size - b->count, size);

    b->put = (b->put + size_ok) % b->size;
    b->count += size_ok;

    return size_ok;
}

// ////////////////////////////////////////////////////////////////
//
// Get data area
//
int readBufBeginGet(ReadBuf *b, u_char **ptr)
{
    if (b->count) {
        *ptr = b->data + (b->put - b->count + b->size) % b->size;
    }
    return b->count;
}

// ////////////////////////////////////////////////////////////////
//
// Proceed 'read' pointer
//
int readBufEndGet(ReadBuf *b, int size)
{
    int size_ok = min(b->count, size);

    b->count -= size_ok;

    return size_ok;
}

} // namespace Flx
