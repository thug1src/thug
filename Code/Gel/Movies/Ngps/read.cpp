#include <libmpeg.h>
#include <core/defines.h>
#include <core/macros.h>
#include <core/singleton.h>
#include "gel/movies/ngps/p_movies.h"
#include "gel/movies/ngps/readbuf.h"
#include "gel/movies/ngps/videodec.h"
#include "gel/movies/ngps/defs.h"
#include "gel/movies/ngps/audiodec.h"

namespace Flx
{



static int copy2area(u_char *pd0, int d0, u_char *pd1, int d1,
    u_char *ps0, int s0, u_char *ps1, int s1);

// ////////////////////////////////////////////////////////////////
//
// Stream callback function for MPEG2 video stream
//
int videoCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data)
{
	
    
	CHECK_MOVIE_MEM;
	
	ReadBuf *rb = (ReadBuf*)data;
    u_char *ps0 = str->data;
    u_char *ps1 = rb->data;
    int s0 = min( ( int )rb->data + ( int )rb->size - ( int )str->data, ( int )str->len);
    int s1 = str->len - s0;
    u_char *pd0;
    u_char *pd1;
    u_char *pd0Unc;
    u_char *pd1Unc;
    int d0, d1;
    int len;

    videoDecBeginPut(& MOVIE_MEM_PTR videoDec, &pd0, &d0, &pd1, &d1);
    pd0Unc = (u_char*)UncAddr(pd0);
    pd1Unc = (u_char*)UncAddr(pd1);

    len = copy2area(pd0Unc, d0, pd1Unc, d1, ps0, s0, ps1, s1);

    // set PTS
    if (len > 0) {
	if (!videoDecPutTs(& MOVIE_MEM_PTR videoDec, str->pts, str->dts, pd0, len)) {
	    ErrMessage("pts buffer overflow\n");
	}
    }

    videoDecEndPut(& MOVIE_MEM_PTR videoDec, len);

    // ////////////////////////////////////////////
    //
    // Return 0 if no data is put
    //
    return (len > 0)? 1: 0;
}

// ////////////////////////////////////////////////////////////////
//
// Stream callback function for PS2 PCM stream
//
int pcmCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data)
{
    ReadBuf *rb = (ReadBuf*)data;
    u_char *ps0 = str->data;
    u_char *ps1 = rb->data;
    int s0;
    int s1;
    u_char *pd0;
    u_char *pd1;
    int d0, d1;
    int len;
    int ret;

    // skip
    // sub_stream_id
    ps0 = str->data + 4;
    if (ps0 >= rb->data + rb->size) {
    	ps0 -= rb->size;
    }
    len = str->len - 4;

    ps1 = rb->data;
    s0 = min(rb->data + rb->size - ps0, len);
    s1 = len - s0;

    audioDecBeginPut(& MOVIE_MEM_PTR audioDec, &pd0, &d0, &pd1, &d1);
    ret = copy2area(pd0, d0, pd1, d1, ps0, s0, ps1, s1);

    audioDecEndPut(& MOVIE_MEM_PTR audioDec, ret);

    // ////////////////////////////////////////////
    //
    // Return 0 if no data is put
    //
    return (ret > 0)? 1: 0;
}

// ////////////////////////////////////////////////////////////////
//
// Copy two areas
//
static int copy2area(u_char *pd0, int d0, u_char *pd1, int d1,
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
