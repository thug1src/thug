#ifndef _READBUF_H_
#define _READBUF_H_

#include <eetypes.h>

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/macros.h>
#include <core/singleton.h>


#define READ_UNIT_SIZE (64*1024)
#define N_READ_UNIT     5

// ////////////////////////////////////////////////////////////////
//
// Read buffer
//
struct ReadBuf{
    u_char data[N_READ_UNIT * READ_UNIT_SIZE];
    int put;
    int count;
    int size;
};

namespace Flx
{



// ////////////////////////////////////////////////////////////////
//
// Functions
//
void readBufCreate(ReadBuf *buff);
void readBufDelete(ReadBuf *buff);
int readBufBeginPut(ReadBuf *buff, u_char **ptr);
int readBufEndPut(ReadBuf *buff, int size);
int readBufBeginGet(ReadBuf *buff, u_char **ptr);
int readBufEndGet(ReadBuf *buff, int size);

} // namespace Flx

#endif // _READBUF_H_

