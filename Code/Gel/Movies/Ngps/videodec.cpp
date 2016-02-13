#include <stdio.h>
#include <eetypes.h>
#include "gel/movies/ngps/p_movies.h"
#include "gel/movies/ngps/defs.h"
#include "gel/movies/ngps/vobuf.h"
#include "gel/movies/ngps/videodec.h"
#include "gel/movies/ngps/disp.h"

namespace Flx
{



void videoDecMain( void * dummy);
int decBs0(VideoDec *vd);
int mpegError(sceMpeg *mp, sceMpegCbDataError *cberror, void *anyData);
int mpegNodata(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData);
int mpegStopDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData);
int mpegRestartDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData);
int mpegTS(sceMpeg *mp, sceMpegCbDataTimeStamp *cbts, void *anyData);
static int cpy2area(u_char *pd0, int d0, u_char *pd1, int d1,
    u_char *ps0, int s0, u_char *ps1, int s1);

extern int vblankCount;

// ////////////////////////////////////////////////////////////////
//
//  Create video decoder
//
int videoDecCreate(VideoDec *vd,
    u_char *mpegWork, int mpegWorkSize,
    u_long128 *data, u_long128 *tag,
    int tagSize, TimeStamp *pts, int n_pts)
{
    // Create sceMpeg
    sceMpegCreate(&vd->mpeg, mpegWork, mpegWorkSize);

    // Add Callbacks
    sceMpegAddCallback(&vd->mpeg,
    	sceMpegCbError, (sceMpegCallback)mpegError, NULL);
    sceMpegAddCallback(&vd->mpeg,
    	sceMpegCbNodata, mpegNodata, NULL);
    sceMpegAddCallback(&vd->mpeg,
    	sceMpegCbStopDMA, mpegStopDMA, NULL);
    sceMpegAddCallback(&vd->mpeg,
    	sceMpegCbRestartDMA, mpegRestartDMA, NULL);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbTimeStamp,
    	(sceMpegCallback)mpegTS, NULL);

    videoDecReset(vd);

    // Create input video buffer
    viBufCreate(&vd->vibuf, data, tag, tagSize, pts, n_pts);

    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Set decode mode
//
void videoDecSetDecodeMode(VideoDec *vd, int ni, int np, int nb)
{
    sceMpegSetDecodeMode(&vd->mpeg, ni, np, nb);
}

// ////////////////////////////////////////////////////////////////
//
//  Choose stream to be decoded
//
int videoDecSetStream(VideoDec *vd, int strType, int ch,
	sceMpegCallback cb, void *data)
{
    sceMpegAddStrCallback(&vd->mpeg, ( sceMpegStrType )strType, ch, cb, data);
    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Get areas to put data in video input buffer
//
void videoDecBeginPut(VideoDec *vd,
	u_char **ptr0, int *len0, u_char **ptr1, int *len1)
{
    viBufBeginPut(&vd->vibuf, ptr0, len0, ptr1, len1);
}

// ////////////////////////////////////////////////////////////////
//
//  Proceed pointers of video input buffer
//
void videoDecEndPut(VideoDec *vd, int size)
{
    viBufEndPut(&vd->vibuf, size);
}

// ////////////////////////////////////////////////////////////////
//
//  Reset video decoder
//
void videoDecReset(VideoDec *vd)
{
    vd->state = VD_STATE_NORMAL;
}

// ////////////////////////////////////////////////////////////////
//
//  Delete video decoder
//
int videoDecDelete(VideoDec *vd)
{
    viBufDelete(&vd->vibuf);
    sceMpegDelete(&vd->mpeg);

    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Abort decoding
//
void videoDecAbort(VideoDec *vd)
{
    vd->state = VD_STATE_ABORT;
}

// ////////////////////////////////////////////////////////////////
//
//  Get state of video decoder
//
u_int videoDecGetState(VideoDec *vd)
{
    return vd->state;
}

// ////////////////////////////////////////////////////////////////
//
//  Set state of video decoder
//
u_int videoDecSetState(VideoDec *vd, u_int state)
{
    u_int old = vd->state;

    vd->state = state;

    return old;
}

// ////////////////////////////////////////////////////////////////
//
//  Put time stamp to the video decoder
//
int videoDecPutTs(VideoDec *vd, long pts_val,
    long dts_val, u_char *start, int len)
{
    TimeStamp ts;

    // Set PTS
    ts.pts = pts_val;
    ts.dts = dts_val;
    ts.pos = start - (u_char*)vd->vibuf.data;
    ts.len = len;
    return viBufPutTs(& MOVIE_MEM_PTR videoDec.vibuf, &ts);
}

// ////////////////////////////////////////////////////////////////
//
//  Data size in video input buffer
//
int videoDecInputCount(VideoDec *vd)
{
    return viBufCount(&vd->vibuf);
}

// ////////////////////////////////////////////////////////////////
//
//  Empty space size in video input buffer
//
int videoDecInputSpaceCount(VideoDec *vd)
{
    u_char *ptr0;
    u_char *ptr1;
    int len0, len1;

    videoDecBeginPut(vd, &ptr0, &len0, &ptr1, &len1);

    return len0 + len1;
}

// ////////////////////////////////////////////////////////////////
//
//  Flush video decoder
//
int videoDecFlush(VideoDec *vd)
{
    u_char *pd0;
    u_char *pd1;
    u_char *pd0Unc;
    u_char *pd1Unc;
    u_char seq_end_code[4] = {0x00, 0x00, 0x01, 0xb7};
    int d0, d1;
    int len;

    videoDecBeginPut(vd, &pd0, &d0, &pd1, &d1);

    if (d0 + d1 < 4) {
    	return 0;
    }

    pd0Unc = (u_char*)UncAddr(pd0);
    pd1Unc = (u_char*)UncAddr(pd1);

    len = cpy2area(pd0Unc, d0, pd1Unc, d1, seq_end_code, 4, NULL, 0);

    videoDecEndPut(& MOVIE_MEM_PTR videoDec, len);

    viBufFlush(&vd->vibuf);

    if (vd->state == VD_STATE_NORMAL) {
	vd->state = VD_STATE_FLUSH;
    }

    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Check to see if input buffer and reference image buffer are
//  flushed
//
int videoDecIsFlushed(VideoDec *vd)
{
    return (videoDecInputCount(vd) == 0)
    	&& sceMpegIsRefBuffEmpty(&vd->mpeg);
}

// ////////////////////////////////////////////////////////////////
//
//  Main function of decode thread
//
void videoDecMain( void * dummy)
{
	
	
	CHECK_MOVIE_MEM;
	
//	Dbg_MsgAssert( ( & MOVIE_MEM_PTR videoDec ) == vd, ( "What the fuck assface?" ) );
    viBufReset(& MOVIE_MEM_PTR videoDec.vibuf);
//    viBufReset(&vd->vibuf);

    voBufReset(& MOVIE_MEM_PTR voBuf);

    decBs0(& MOVIE_MEM_PTR videoDec);

    while ( MOVIE_MEM_PTR voBuf.count)
    	;

     videoDecSetState( & MOVIE_MEM_PTR videoDec, VD_STATE_END);
}

// ////////////////////////////////////////////////////////////////
//
// Decode bitstream using MPEG decoder
//
// return value
//     1: normal end
//     -1: aborted
int decBs0(VideoDec *vd)
{
	
    
	CHECK_MOVIE_MEM;
	
	VoData *voData;
    sceIpuRGB32 *rgb32;
    int ret;
    int status = 1;
    sceMpeg *mp = &vd->mpeg;

    // ////////////////////////////
    // 
    //  Main loop to decode MPEG bitstream
    // 
    while (!sceMpegIsEnd(&vd->mpeg)) {

	if (videoDecGetState(vd) == VD_STATE_ABORT) {
	    status = -1;
	    printf("decode thread: aborted\n");
	    break;
	}

	// ////////////////////////////////////////////////
	// 
	//  Get next available ouput buffer from voBuf
	//  switch to another thread if voBuf is full
	//
	while (!(voData = voBufGetData(& MOVIE_MEM_PTR voBuf))) {
	    switchThread();
	}
	rgb32 = (sceIpuRGB32*)voData->v;

	// ////////////////////////////////////////////////
	// 
	//  Get decoded picture in sceIpuRGB32 format
	//
        ret = sceMpegGetPicture(mp, rgb32, MAX_WIDTH/16 * MAX_HEIGHT/16);

        if (ret < 0) {
	    ErrMessage("sceMpegGetPicture() decode error");
	}

	if (mp->frameCount == 0) {
	    int i;
	    int image_w, image_h;

	    image_w = mp->width;
	    image_h = mp->height;

	    for (i = 0; i <  MOVIE_MEM_PTR voBuf.size; i++) {

	      // packet with texture data
	      setImageTag( MOVIE_MEM_PTR voBuf.tag[i].v[0],  MOVIE_MEM_PTR voBuf.data[i].v,
			  0, image_w, image_h);
	      
	      // packet without texture data
	      setImageTag( MOVIE_MEM_PTR voBuf.tag[i].v[1],  MOVIE_MEM_PTR voBuf.data[i].v,
			  1, image_w, image_h);
	    }
	}

	// ////////////////////////////
	// 
	//  Increment video output buffer count
	// 
	voBufIncCount(& MOVIE_MEM_PTR voBuf);

	switchThread();
    }

    sceMpegReset(mp);

    return status;
}

// ////////////////////////////////////////////////////////////////
//
//  Callback function for sceMpegCbError
//
int mpegError(sceMpeg *mp, sceMpegCbDataError *cberror, void *anyData)
{
    printf("%s\n", cberror->errMessage);
    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Callback function for sceMpegCbNodata
//
int mpegNodata(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData)
{
    switchThread();
    viBufAddDMA(& MOVIE_MEM_PTR videoDec.vibuf);
    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Callback function for sceMpegCbStopDMA
//
int mpegStopDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData)
{
    // Stop DMA
    viBufStopDMA(& MOVIE_MEM_PTR videoDec.vibuf);
    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Callback function for sceMpegCbRestartDMA
//
int mpegRestartDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData)
{
    // Restart DMA
    viBufRestartDMA(& MOVIE_MEM_PTR videoDec.vibuf);
    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Callback function for sceMpegCbTimeStamp
//	retruns PTS and DTS
//
int mpegTS(sceMpeg *mp, sceMpegCbDataTimeStamp *cbts, void *anyData)
{
    TimeStamp ts;

    viBufGetTs(& MOVIE_MEM_PTR videoDec.vibuf, &ts);
    cbts->pts = ts.pts;
    cbts->dts = ts.dts;
    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  copy 2 areas
//
static int cpy2area(u_char *pd0, int d0, u_char *pd1, int d1,
    u_char *ps0, int s0, u_char *ps1, int s1)
{
    if (d0 + d1 < s0 + s1) {
        return 0;
    }

    if (s0 >= d0) {
    	memcpy(pd0,		ps0,		d0);
    	memcpy(pd1,		ps0 + d0,	s0 - d0);
    	memcpy(pd1 + s0 - d0,	ps1,		s1);
    } else { // s0 < d0
    	if (s1 >= d0 - s0) {
	    memcpy(pd0,		ps0,		s0);
	    memcpy(pd0 + s0,	ps1,		d0 - s0);
	    memcpy(pd1,		ps1 + d0 - s0,	s1 - (d0 - s0));
	} else { // s1 < d0 - s0
	    memcpy(pd0,		ps0,		s0);
	    memcpy(pd0 + s0,	ps1,		s1);
	}
    }
    return s0 + s1;
}


} // namespace Flx
