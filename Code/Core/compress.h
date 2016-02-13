#ifndef __CORE_COMPRESS_H
#define __CORE_COMPRESS_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

int Encode(char *pIn, char *pOut, int bytes_to_read, bool print_progress);
unsigned char *DecodeLZSS(unsigned char *pIn, unsigned char *pOut, int Len);

#endif

