#ifndef _STRFILE_H_
#define _STRFILE_H_

#include <eetypes.h>
#include <libcdvd.h>

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/macros.h>
#include <core/singleton.h>

// ////////////////////////////////////////////////////////////////
//
//  Structure to read data from CD/DVD or HD
//
struct StrFile{
    int isOnCD;		// CD/DVD or HD
    int size;

    sceCdlFILE fp;	// for CD/DVD stream
    u_char *iopBuf;

    int fd;		// for HD stream
};

namespace Flx
{



int strFileOpen(StrFile *file, char *filename, int pIopBuff);
int strFileClose(StrFile *file);
int strFileRead(StrFile *file, void *buff, int size);

} // namespace Flx

#endif // _STRFILE_H_
