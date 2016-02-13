#ifndef _VIDEODEC_H_
#define _VIDEODEC_H_

#include <libmpeg.h>

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/macros.h>
#include <core/singleton.h>

#include "gel/movies/ngps/vibuf.h"

// ////////////////////////////////////////////////////////////////
//
//  Video decoder state
//
#define VD_STATE_NORMAL    0
#define VD_STATE_ABORT     1
#define VD_STATE_FLUSH     2
#define VD_STATE_END       3

namespace Flx
{



// ////////////////////////////////////////////////////////////////
//
//  Video Decoder
//
struct VideoDec{
    sceMpeg mpeg;	// MPEG decoder
    ViBuf vibuf;	// video input buffer
    u_int state;	// video decoder state
    int sema;		// semaphore

    int hid_endimage;	// handler to check the end of image transfer
    int hid_vblank;	// vlbank handler

};

// ////////////////////////////////////////////////////////////////
//
//  Functions
//
void videoDecReset(VideoDec *vd);
int videoDecCreate(VideoDec *vd,
    u_char *mpegWork, int mpegWorkSize,
    u_long128 *data, u_long128 *tag,
    int tagSize, TimeStamp *pts, int n_pts);
int videoDecDelete(VideoDec *vd);
void videoDecAbort(VideoDec *vd);
u_int videoDecGetState(VideoDec *vd);
u_int videoDecSetState(VideoDec *vd, u_int state);
int videoDecInputCount(VideoDec *vd);
int videoDecInputSpaceCount(VideoDec *vd);
void videoDecSetDecodeMode(VideoDec *vd, int ni, int np, int nb);
int videoDecFlush(VideoDec *vd);
int videoDecIsFlushed(VideoDec *vd);
int videoDecSetStream(VideoDec *vd, int strType, int ch,
	sceMpegCallback h, void *data);
void videoDecBeginPut(VideoDec *vd,
	u_char **ptr0, int *len0, u_char **ptr1, int *len1);
void videoDecEndPut(VideoDec *vd, int size);
int videoDecPutTs(VideoDec *vd, long pts_val,
    long dts_val, u_char *start, int len);

} // namespace Flx

#endif // _VIDEODEC_H_
