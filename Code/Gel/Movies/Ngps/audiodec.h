#ifndef _AUDIODEC_H_
#define _AUDIODEC_H_

#include <eetypes.h>

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/macros.h>
#include <core/singleton.h>

// ///////////////////////////////////////////////////////
//
// Audio decoder state
//
#define AU_STATE_INIT		0
#define AU_STATE_PRESET		1
#define AU_STATE_PLAY		2
#define AU_STATE_PAUSE		3

#define AU_HDR_SIZE		( ( int )(sizeof(SpuStreamHeader) 					+ sizeof(SpuStreamBody)) )

// ///////////////////////////////////////////////////////
//
// Spu stream header
//
struct SpuStreamHeader{
    char id[4];		// 'S''S''h''d'
    int size;		// 24
    int type;		// 0: 16bit big endian
    			// 1: 16bit little endian
			// 2: SPU2-ADPCM (VAG) 
    int rate;		// sampling rate
    int ch;		// number of channels
    int interSize;	// interleave size ... needs to be 512
    int loopStart;	// loop start block address
    int loopEnd;	// loop end block sddress
};

// ///////////////////////////////////////////////////////
//
// Spu stream body
//
struct SpuStreamBody{
    char id[4];		// 'S''S''b''d'
    int size;		// size of audio data
};

// ///////////////////////////////////////////////////////
//
// Audio decoder
//
struct AudioDec{

    int state;

    // header of ADS format
	struct SpuStreamHeader sshd;
	struct SpuStreamBody   ssbd;
    int hdrCount;

    // audio buffer
    u_char *data;
    int put;
    int count;
    int size;
    int totalBytes;

    // buffer on IOP
    int iopBuff;
    int iopBuffSize;
    int iopLastPos;
    int iopPausePos;
    int totalBytesSent;
    int iopZero;

};

namespace Flx
{



// ///////////////////////////////////////////////////////
//
// Functions
//
int audioDecCreate(
    struct AudioDec *ad,
    u_char *buff,
    int buffSize,
    int iopBuffSize,
	int pIopBuf
);
int audioDecDelete( struct AudioDec *ad);
void audioDecBeginPut( struct AudioDec *ad,
	u_char **ptr0, int *len0, u_char **ptr1, int *len1);
void audioDecEndPut(struct AudioDec *ad, int size);
int audioDecIsPreset(struct AudioDec *ad);
void audioDecStart(struct AudioDec *ad);
int audioDecSendToIOP(struct AudioDec *ad);
void audioDecReset(struct AudioDec *ad);
void audioDecPause(struct AudioDec *ad);
void audioDecResume(struct AudioDec *ad);

} // namespace Flx

#endif _AUDIODEC_H_



