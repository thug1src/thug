/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			ps2 movies												**
**																			**
**	File name:		gel/movies/ngps/p_movies.h 								**
**																			**
**	Created: 		5/14/01	-	mjd											**
**																			**
*****************************************************************************/

#ifndef __P_MOVIES_H
#define __P_MOVIES_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/singleton.h>
#include <core/list.h>
#include <core/macros.h>
#include <libmpeg.h>
#include <libpad.h>

#include "gel/movies/ngps/defs.h"
#include "gel/movies/ngps/vobuf.h"
#include "gel/movies/ngps/vibuf.h"
#include "gel/movies/ngps/strfile.h"
#include "gel/movies/ngps/readbuf.h"
#include "gel/movies/ngps/videodec.h"
#include "gel/movies/ngps/audiodec.h"

namespace Flx
{



#define MOVIE_STACK_SIZE    (16*1024)
#define MOVIE_DEF_STACK_SIZE    2048
#define MAX_MBX		(MAX_WIDTH/16)
#define MAX_MBY		(MAX_HEIGHT/16)
#define IOP_BUFF_SIZE (12288*2) // 512 * 48
#define MOVIE_ABORTED SCE_PADRdown
//#define DEF_PRIORITY       32
#define ERR_STOP while(1)

#define MPEG_WORK_SIZE ( SCE_MPEG_BUFFER_SIZE(MAX_WIDTH, MAX_HEIGHT) )
#define AUDIO_BUFF_SIZE ( IOP_BUFF_SIZE * 2 )

#define ZERO_BUFF_SIZE	0x800
	  
// send in size, returns the size needed to create a 64-byte boundary:
#define PAD_FOR_64( x ) ( 64 - ( ( x ) & 63 ) )
// send in size, returns the size to the next 64 byte boundary:
#define PADDED_64( x ) ( ( x ) + PAD_FOR_64( x ) )

// This structure contains everything that was previously just
// global (and taking up loads of memory)...
struct SMovieMem{

	u_long128 packetBase[ 6 ];
	char pad665[ PAD_FOR_64( sizeof( u_long128 ) * 6 ) ];
		 
	char _0_buf [ ZERO_BUFF_SIZE ];
	char pad666[ PAD_FOR_64( ZERO_BUFF_SIZE ) ];
	
	// ******* NOTE ::: KEEP ALL THESE ON 64 - BYTE BOUNDARIES AND SHIT!! ********

	// These variables could be accessed from Uncached Area
	VoData voBufData[ N_VOBUF ];
	char pad0[ PAD_FOR_64( ( sizeof( VoData ) * N_VOBUF ) ) ];

	VoTag voBufTag[ N_VOBUF ];
	char pad1[ PAD_FOR_64( ( sizeof( VoTag ) * N_VOBUF ) ) ];

	u_long128 viBufTag[ VIBUF_SIZE + 1 ];
	char pad2[ PAD_FOR_64( ( sizeof( u_long128 ) * ( VIBUF_SIZE + 1 ) ) ) ];

	// -------------- this needs to be 64 byte boudary -------------------
	
	// These variables are NOT accessed from Uncached Area
	u_char mpegWork[ PADDED_64( MPEG_WORK_SIZE ) ];
	char defStack[ PADDED_64( MOVIE_DEF_STACK_SIZE )];
	u_char audioBuff[PADDED_64( IOP_BUFF_SIZE*2 )];
	u_long128 viBufData[ PADDED_64( VIBUF_SIZE * VIBUF_ELM_SIZE/16 ) ];
	char videoDecStack[ PADDED_64 ( MOVIE_STACK_SIZE )];
	TimeStamp timeStamp[ VIBUF_TS_SIZE ];
	char pad3[ PAD_FOR_64( sizeof( TimeStamp ) * VIBUF_TS_SIZE ) ];

//	u_long128 controller_dma_buf[ scePadDmaBufferMax ];
//	char pad4[ PAD_FOR_64( ( sizeof( u_long128 ) * ( scePadDmaBufferMax ) ) ) ];

	struct ReadBuf readBuf;
	char pad5[ PAD_FOR_64( sizeof( ReadBuf ) ) ];
	
	struct StrFile infile;
	char pad6[ PAD_FOR_64( sizeof( StrFile ) ) ];
	struct VideoDec videoDec;
	char pad7[ PAD_FOR_64( sizeof( VideoDec ) ) ];
	struct AudioDec audioDec;
	char pad8[ PAD_FOR_64( sizeof( AudioDec ) ) ];
	struct VoBuf voBuf;
};

extern SMovieMem *gpMovieMem;

#define MOVIE_MEM_PTR	gpMovieMem->
#define CHECK_MOVIE_MEM	Dbg_MsgAssert( gpMovieMem, ( "Movie Memory not initialized." ) )

void PMovies_PlayMovie( const char *pName );

void FuckUpVram( char color ); // testing what the fuck vram funcs are doing...

} // namespace Flx

#endif	// __P_MOVIES_H



