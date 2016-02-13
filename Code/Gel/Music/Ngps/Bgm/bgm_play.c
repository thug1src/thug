#include <stdio.h>
#include <sys/file.h>
#include <kernel.h>
#include <string.h>
#include <libsd.h>
#include <libcdvd.h>
#include "bgm_i.h"

#define DEBUGGING_PLEASE 0
#if DEBUGGING_PLEASE
#define Dbg_Printf(A...)  printf(##A) 		
#else
#define Dbg_Printf(A...)
#endif

// wav format ------------------------
#define RIFF_HEADER_SIZE 44
typedef struct {
	unsigned char     chunkID[4];
	unsigned int      chunkSize;
	unsigned short*   data;
} DATAHeader;

typedef struct {
	unsigned char     chunkID[4];
	unsigned int      chunkSize;
	unsigned short    waveFmtType;
	unsigned short    channel;
	unsigned int      samplesPerSec;
	unsigned int      bytesPerSec;
	unsigned short    blockSize;
	unsigned short    bitsPerSample;
} FMTHeader;

typedef struct {
	unsigned char     chunkID[4];
	unsigned int      chunkSize;
	unsigned char     formType[4];
	FMTHeader         fmtHdr;
	DATAHeader        dataHdr;
} RIFFHeader;
//------------------------------------

typedef struct {
	unsigned int     size;
//	unsigned int     offset;
	unsigned int     pos;
} BGM_WAVE;


BGM_WAVE gWave[2];

int gMemInitializedForCD = 0;
short gBgmPause[2] = { 0, 0 };
short gBgmPreload[ 2 ] = { 0, 0 };
char gFileFromCD[ 2 ] = { 0, 0 };
char gBgmVolumeSet[2] = { 0, 0 };
volatile char gFileOpened[2] = { 0, 0 };
volatile char gBgmIntr[2] = { 0, 0 };
int gThid = 0;
int gSem = 0;
int gFd[2];
int gBuffRaw[2];
int gBuffSpu[2] = {0, 0};
int gRPacketSize[2];
int gSPacketSize[2];
int gAllockedSize[2];
int gBgmVolume[2];
int gBgmMode[2] = { BGM_MODE_REPEAT_OFF, BGM_MODE_REPEAT_OFF };
int gBgmStereoMode[ 2 ] = { 0, 0 };

#define MAX_FILENAME_LENGTH		50
char gFilename[ 2 ][ MAX_FILENAME_LENGTH ];

// Larger buffers will prevent skipping when loading in the background...
// The more streams, the larger the buffers have to be.
// Reading from the CDVD will be faster than debug stations from the host over the network,
// so don't freak if you notice corruption on a dev system w/o CD.
#define SECTOR_SIZE					2048
#define NUM_SECTORS_PER_READ		2
#define BUFFER_SIZE_FOR_HOST_FILE	( 2048 * 132 ) // multiple of 6
#define BUFFER_SIZE_FOR_CD_FILE		( SECTOR_SIZE * NUM_SECTORS_PER_READ * 6 )

#define RING_BUFFER_NUM_SECTORS		48
#define RING_BUFFER_ALLIGNMENT		64
#define RING_BUFFER_NUM_PARTITIONS	3
unsigned char gRingBuffer[ 2 ][ RING_BUFFER_NUM_SECTORS * 2048 ] __attribute__((aligned ( RING_BUFFER_ALLIGNMENT )));
unsigned char gCDBuffSpu[ 2 ][ BUFFER_SIZE_FOR_CD_FILE ]__attribute__((aligned ( 64 )));

int  _BgmPlay( int status );
void _BgmRaw2Spu(u_long *src, u_long *dst,  u_long block_count );
void _BgmRaw2SpuMono(u_long *src, u_long *dst,  u_long block_count );
int	BgmPreLoad( int ch, int status );
void SetStereoOn( int ch, int stereo );

static int IntFunc0( void* common )
{
	gBgmIntr[0] = 1;
	iSignalSema( gSem );
	return 1;  //-- Required for re-enabling interrupts
}

static int IntFunc1( void* common )
{
	gBgmIntr[1] = 1;
	iSignalSema( gSem );
	return 1;  //-- Required for re-enabling interrupts
}


static int makeMyThread( void )
{
    struct ThreadParam param;
    int	thid;

    param.attr         = TH_C;
    param.entry        = _BgmPlay;
    param.initPriority = BASE_priority-3;
    param.stackSize    = 0x800;
    param.option = 0;

    /* Create thread */
	thid = CreateThread(&param);

	return thid;
}

static int makeMySem( void )
{
    struct SemaParam sem;

    sem.initCount = 0;
    sem.maxCount = 1;
    sem.attr = AT_THFIFO;

    /* Create semaphore.  */
    return CreateSema(&sem);
}


int BgmRaw2Spu( int ch, int which )
{
    if ( gBgmStereoMode[ ch ] )
	{
		_BgmRaw2Spu( (u_long *)(gBuffRaw[ch]+ (gRPacketSize[ch])*which)
			, (u_long *)(gBuffSpu[ch] + (gSPacketSize[ch])*which)
			, gSPacketSize[ch]/1024 );
	}
	else
	{
		_BgmRaw2SpuMono ((u_long *)(gBuffRaw [ch] + (gRPacketSize [ch]) * which),
				 (u_long *)(gBuffSpu [ch] + (gSPacketSize [ch]) * which),
				 gSPacketSize [ch] / 1024);
    }
	return 0;
}

// returns zero if no longer playing...
int BgmSetVolumeDirect( int ch, unsigned int vol )
{
	sceSdSetParam( (ch)|SD_P_BVOLR, vol>>16 );
	sceSdSetParam( (ch)|SD_P_BVOLL, vol&0xFFFF);
	return ( gBgmMode[ch] & ~BGM_MASK_STATUS );
}



void BgmSdInit(int ch, int status )
{
	// This is already called from the soundFX module.
	//sceSdInit(0);
	return;
}


int BgmInit( int ch, int useCD )
{

	if( gSem == 0 ){
		gSem = makeMySem();
	}
	if( gThid == 0 ){
		gThid = makeMyThread();
		Dbg_Printf("PCM Audio Streamer: create thread ID= %d, ", gThid );
		/* Activate thread  */
		StartThread( gThid, (u_long)NULL );
	}

	if ( !gBuffSpu[ ch ] )
	{
		gAllockedSize[ch] = useCD ? BUFFER_SIZE_FOR_CD_FILE : BUFFER_SIZE_FOR_HOST_FILE;
		if ( useCD )
		{
			gBuffSpu[ ch ] = ( int )gCDBuffSpu[ ch ]; //(int)AllocSysMemory( 0, gAllockedSize[ ch ], NULL );
		}
		else
		{
			gBuffSpu[ ch ] = ( int )AllocSysMemory( 0, gAllockedSize[ ch ], NULL );
		}
		Dbg_Printf(" PCM spu buffer mem: 0x%x - 0x%x\n", gBuffSpu[ch], gBuffSpu[ch]+gAllockedSize[ ch ] );
	}
	// the memory should work for mono or stereo 44k files ( raw buffer always the same... )
	gRPacketSize[ ch ] = gAllockedSize[ ch ] / 6;
	gBuffRaw[ ch ] = ( int )( gBuffSpu[ ch ] + ( gRPacketSize[ ch ] * 4 ) );
	gMemInitializedForCD = useCD;

	return  gThid;
}

void BgmQuit(int ch, int status )
{
	if ( ( gBuffSpu[ ch ] ) && ( gBuffSpu[ ch ] != ( int ) gCDBuffSpu ) )
		FreeSysMemory( (void*)gBuffSpu[ch] );
	gBuffSpu[ch] = 0;
	//-- Frees resources unless another channel is used.
	if( gBuffSpu[1-ch] == 0 ){
		if (gThid != 0) TerminateThread( gThid );
		if (gSem != 0) DeleteSema( gSem );
		if (gThid != 0) DeleteThread( gThid );
		gThid = 0;
		gSem = 0;
	}
	return;
}

/*
void WaitForCDReady( char *description )
{
	printf( "wait %s", description );
	while( SCECdComplete != sceCdDiskReady( 1 ) )
	{
		printf( "." );
	}
	printf( "\n" );
}*/

#define NUM_BYTES_TO_CLEAR	512 // must be divisible by 4!!!

int BgmOpen( int data, char* filename )
{
//	unsigned int channel;
	RIFFHeader wavHdr;
	int offset;
	int *addr;
	int i;
	int ch = data & 1;
	int stereo = data & WAV_STEREO_BIT;
	
	SetStereoOn( ch, stereo );

	for ( i = 0; i < 2; i++ )
	{
		if ( gFileOpened[ i ] )
		{
			printf( "%s: Failed: Audio stream already running: %s\n", filename, gFilename[ i ] );
//			Dbg_Printf( "File already opened on that channel...\n" );
			return -1;
		}
	}
	printf("pcm filename %s\n", filename );
	
	if ( !strncmp( filename, "host", 4 ) )
	{
		if ( gMemInitializedForCD )
		{
			printf( "Loading file from host when mem configured for CD... SOUND QUALITY WILL SUCK.\n" );
			printf( "If calling ezBGM_INIT with useCD (2nd param), send CDROM0 in filename...\n" );
		}
		
		// load the file from the dev system...
		if (( gFd[ch] = open ( filename, O_RDONLY)) < 0)
		{
			ERROR(("file open failed. %s \n", filename));
			return -1;
		}
		
		if ( read (gFd[ch], (unsigned char*)(&wavHdr)
			,RIFF_HEADER_SIZE ) != RIFF_HEADER_SIZE ) {
			ERROR(("file read failed. %s \n", filename));
			return -1;
		}
		if ( wavHdr.fmtHdr.channel == 2 )
		{
			if ( !stereo )
				printf( "WARNING: Playing stereo sound %s in mono mode.\n", filename );
		}
		else if ( stereo )
		{
			printf( "WARNING: Playing mono sound %s in stereo mode.\n", filename );
		}
		
		gWave[ch].size   = wavHdr.dataHdr.chunkSize;
		gWave[ch].pos    = 0;
	
		//--- Seek to start of data
		offset = (unsigned int)&(wavHdr.dataHdr.data) -(unsigned int)&wavHdr;
	
		lseek( gFd[ ch ], offset , SEEK_SET );
	
		gFileFromCD[ ch ] = 0;
		gFileOpened[ ch ] = 1;
		gBgmPreload[ ch ] = 3;
	}
	else
	{
		sceCdlFILE fp;
		sceCdRMode mode;
		int retVal;
		unsigned int stringLength;
		unsigned int lsn;
		int foundCDData = 0;
		int byteIndex;
		// open a stream and load from CD...
		// 1st arg: size in num sectors ( actual size / 2048 ).
//		WaitForCDReady( "st init" );
//		sceCdDiskReady( 0 );
		sceCdStInit( RING_BUFFER_NUM_SECTORS, RING_BUFFER_NUM_PARTITIONS, ( u_int )( gRingBuffer[ ch ] ) ); //+ ( RING_BUFFER_ALLIGNMENT - 1 ) )  );
//		printf( "wait for ready." );
//		sceCdDiskReady( 0 );
//		WaitForCDReady( "init" );
//		printf( "cdSearchFile\n" );
		stringLength = strlen( filename );
		if ( filename[ stringLength + 1 ] == '@' )
		{
			foundCDData = 1;
			lsn = 0;
			for ( byteIndex = 0; byteIndex < 4; byteIndex++ )
			{
				lsn |= ( ( ( unsigned int )( filename[ stringLength + 2 + byteIndex ] ) ) & 255 ) << ( 8 * byteIndex );
			}
		}
		if ( !foundCDData )
		{
			if ( !( retVal = sceCdSearchFile( &fp, filename ) ) )
			{
				ERROR(("cd filesearch failed %d. file %s \n", retVal, filename));
				return -1;
			}
			lsn = fp.lsn;
		}
	
//		WaitForCDReady( "find" );
//		sceCdDiskReady( 0 );
		mode.trycount = 0;
		mode.spindlctrl = SCECdSpinNom;
		mode.datapattern = SCECdSecS2048;
		sceCdStStart( lsn, &mode );
		gFileFromCD[ ch ] = 1;
		gBgmPreload[ ch ] = 3;
		gFileOpened[ ch ] = 1;
		gWave[ ch ].pos = 0;
		gWave[ ch ].size = gRPacketSize[ ch ] * 4;
	}
	
//	printf("wave size %d  offset %d\n", gWave[ch].size, gWave[ch].offset );
	
	BgmSetVolumeDirect( ch, 0 );

	strncpy( gFilename[ ch ], filename, MAX_FILENAME_LENGTH  );

	// clear out the end of the second half of the last buffer...
	addr = ( int * )( gBuffRaw[ ch ] + ( gRPacketSize[ ch ] ) + ( gRPacketSize[ ch ] >> 1 ) );
	for ( i = 0; i < ( ( gRPacketSize[ ch ] >> 1 ) >> 2 ); i++ )
	{
		*( addr++ ) = 0;
	}
	
	if( ch == 0 )
	{
		sceSdSetTransCallback( ch, IntFunc0 );
	}
	else
	{
		sceSdSetTransCallback( ch, IntFunc1 );
	}
		
	return ( gWave[ ch ].size );
}

void _BgmClose(int ch, int status)
{
	if ( !gFileOpened[ ch ] )
	{
		PRINTF(("Calling BgmClose when no file opened on that channel...\n"));
		return;
	}
	Dbg_Printf( "_BgmClose\n" );
	if ( gFileFromCD[ ch ] )
	{
		sceCdStStop( );
	}
	else
	{
		close( gFd[ch] );
		//FreeSysMemory( ( void* )gBuffSpu[ ch ] );
		//gBuffSpu[ ch ] = 0;
	}
	gFileOpened[ ch ] = 0;
	return;
}

void BgmStart(int ch, int status )
{
	int retVal;
	if ( !gFileOpened[ ch ] )
	{
		printf( "trying to start track not loaded.\n" );
		return;
	}
	Dbg_Printf( "starting block trans from bgm start/unpause\n" );
	if ( gBgmPause[ ch ] )
	{
		Dbg_Printf( "Starting music on channel where pause is requested...\n" );
		gBgmPause[ ch ] = 0;
	}
	while( -1 == sceSdBlockTrans( ch, SD_TRANS_MODE_WRITE|SD_BLOCK_LOOP, (u_char*)gBuffSpu[ch], (gSPacketSize[ch]*2) ) )
	{
		for ( retVal = 0; retVal < 10000; retVal++ )
			;
		Dbg_Printf( "failed to start loop.\n" );
	}
	if ( !( gBgmPreload[ ch ] ) )
	{
		BgmSetVolumeDirect(ch, gBgmVolume[ch]);
	}
	gBgmMode[ch] &= BGM_MASK_STATUS;
	gBgmMode[ch] |= BGM_MODE_RUNNING;

	return;
}

void BgmCloseNoWait( int ch, int status )
{
	if ( !gFileOpened[ ch ] )
	{
		PRINTF(("Calling BgmClose when no file opened on that channel...\n"));
		return;
	}
	if ( gBgmPause[ ch ] )
	{
		printf( "stopping music when pause is requested..." );
		return;
	}
	// if the pause has already stopped the callback, it's safe to unload now:
	if ( gBgmMode[ ch ] & BGM_MODE_PAUSE )
	{
		// callback isn't happening right now... can terminate with no worries here:
		Dbg_Printf( "unpausing from BgmClose\n" );
		gBgmMode[ch] &= BGM_MASK_STATUS; // Switch to IDLE mode
		_BgmClose( ch, status );
		Dbg_Printf( "calling _BgmClose from BgmClose\n" );
		return;
	}
	gBgmMode[ch] &= BGM_MASK_STATUS;
	gBgmMode[ch] |= BGM_MODE_TERMINATE;
}

void BgmClose( int ch, int status )
{
	int i;
	BgmCloseNoWait( ch, status );
	printf( "PCM close" );
	while ( ( gBgmMode[ ch ] & BGM_MODE_TERMINATE ) )
	{
		for ( i = 0; i < 20000; i++ )
			;
		printf( "." );
	}
	printf( "\n" );
}

int BgmPreLoad( int ch, int status )
{
	if ( ( status == 666 ) || ( ( gWave[ ch ].size - gWave[ ch ].pos ) < gRPacketSize[ ch ] * 4 ) )
	{
		int i;
		int *addr;
		Dbg_Printf( "preloading at end of song. zero buffers.\n" );
		// we're at the end of the song... fill up read buffers with zero,
		// and convert...
		addr = ( int * )( gBuffRaw[ ch ] );
		for ( i = 0; i < ( ( gRPacketSize[ ch ] * 2 ) >> 2 ); i++ )
		{
			*( addr++ ) = 0;
		}
		// fill up the spu with both buffers...
		BgmRaw2Spu( ch, 0 );
		BgmRaw2Spu( ch, 1 );
		return ( 0 );
	}
	
	// it's still in preload mode... no need to preload ourselves...
	if ( gBgmPreload[ ch ] == 3 )
		return ( 0 );

	if ( gFileFromCD[ ch ] )
	{
		unsigned int err;
		sceCdStRead( NUM_SECTORS_PER_READ * 2, ( u_int * )gBuffRaw[ ch ], STMBLK, &err );
		if ( err )
		{
			printf( "PCM A Disk error code 0x%08x\n", err );
		}

		// fill up the spu with both buffers...
		BgmRaw2Spu( ch, 0 );
		BgmRaw2Spu( ch, 1 );
		
		// fill up the buffers again...
		sceCdStRead( NUM_SECTORS_PER_READ * 2, ( u_int * )gBuffRaw[ ch ], STMBLK, &err );
		if ( err )
		{
			printf( "PCM B Disk error code 0x%08x\n", err );
		}
		
		gWave[ ch ].pos += gRPacketSize[ ch ] * 2;
		return 0;
	}
	// reading from a file...
	if ( read (gFd[ch], (unsigned char*)(gBuffRaw[ch])
	, gRPacketSize[ch]*2 ) != gRPacketSize[ch]*2 ) {
		ERROR (("BgmPreLoad: read failed \n")); return -1;
	}

	BgmRaw2Spu( ch, 0 );
	BgmRaw2Spu( ch, 1 );

	if ( read (gFd[ch], (unsigned char*)(gBuffRaw[ch])
	, (gRPacketSize[ch]*2) ) != gRPacketSize[ch]*2) {
		ERROR (("BgmPreLoad: read failed \n")); return -1;
	}

	gWave[ch].pos += gRPacketSize[ch]*4;

	return 0;
}

void _BgmPause(int ch, int status)
{
	Dbg_Printf( "stopping spu block trans in _BgmPause\n" );
	sceSdBlockTrans( ch, SD_TRANS_MODE_STOP, NULL, 0 );
	gBgmMode[ch] &= BGM_MASK_STATUS; // Switch to PAUSE mode
	gBgmMode[ch] |= BGM_MODE_PAUSE;
	return;
}

void _BgmStopAndUnload(int ch, int status)
{
	Dbg_Printf( "stopping block trans from _BgmStopAndUnload\n" );
	sceSdBlockTrans( ch, SD_TRANS_MODE_STOP, NULL, 0 );
	gBgmMode[ch] &= BGM_MASK_STATUS; // Switch to IDLE mode
	Dbg_Printf( "bgm close from _BgmStopAndUnload\n" );
	_BgmClose( ch, status );
	return;
}

// should be fucking called "BgmPause"
void BgmStop( int ch, unsigned int vol )
{
	int i;

	if ( gBgmMode[ ch ] & BGM_MODE_TERMINATE )
	{
		Dbg_Printf( "trying to pause PCM when stopped.\n" );
		return;
	}
	if ( !gBgmMode[ ch ] & BGM_MODE_RUNNING )
	{
		Dbg_Printf( "trying to pause PCM when not running.\n" );
		return;
	}
	gBgmPause[ch] = 1;
	printf( "waiting for pause" );
	while ( !( gBgmMode[ ch ] & BGM_MODE_PAUSE ) )
	{
		for ( i = 0; i < 10000; i++ )
			;
		printf( "." );
	}
	printf( "\n" );
	// don't set the flag until the callback has paused it:
//	gBgmMode[ch] &= BGM_MASK_STATUS;
//	gBgmMode[ch] |= BGM_MODE_PAUSE;
	return;
}


int BgmSetVolume( int ch, unsigned int vol )
{
	gBgmVolumeSet[ch] = 1;
	gBgmVolume[ch] = vol;
	return ( gBgmMode[ch] & ~BGM_MASK_STATUS );
}


unsigned int BgmGetMode( int ch, int status )
{
	return gBgmMode[ch];
}

void BgmSeek( int ch, unsigned int value )
{
//	lseek(gFd[ch], gWave[ch].offset+value, SEEK_SET );
//	gWave[ch].pos = value;
	printf( "seek not supported\n" );
	return;
}

void SetStereoOn( int ch, int stereo )
{
    if ( stereo )
	{
		gSPacketSize[ ch ] = gAllockedSize[ ch ] / 6;
		gBgmStereoMode[ ch ] = 1;
	}
	else
	{
		gSPacketSize[ ch ] = gAllockedSize [ ch ] / 3;
		gBgmStereoMode[ ch ] = 0;
	}
}

void PreloadFile( int which, int ch )
{
	switch ( gBgmPreload[ ch ] )
	{
		case ( 3 ):
			if ( gFileFromCD[ ch ] )
			{
				RIFFHeader *pWavHdr;
				int i;
				int offset;
				unsigned char *pData;
				pWavHdr = ( RIFFHeader * )( gBuffRaw[ ch ]+gRPacketSize[ch]*which );
				if ( pWavHdr->dataHdr.chunkSize < gWave[ ch ].size )
				{
					printf( "BIG WARNING:  Cd audio file currently playing is TOO SMALL!\n" );
				}
				if ( pWavHdr->fmtHdr.channel == 2 )
				{
					if ( !gBgmStereoMode[ ch ] )
						printf( "WARNING: Playing stereo sound %s in mono mode.\n", gFilename[ ch ] );
				}
				else if ( gBgmStereoMode[ ch ] )
				{
					printf( "WARNING: Playing mono sound %s in stereo mode.\n", gFilename[ ch ] );
				}
				
				SetStereoOn( ch, pWavHdr->fmtHdr.channel == 2 );
				gWave[ch].size   = pWavHdr->dataHdr.chunkSize;
				//gWave[ch].offset = (unsigned int)&( pWavHdr->dataHdr.data ) - ( unsigned int )pWavHdr;
				offset = (unsigned int)&( pWavHdr->dataHdr.data ) - ( unsigned int )pWavHdr;
				// clear out the area where the header is... don't want some loud obnoxious sounds...
				pData = ( unsigned char * )( gBuffRaw[ ch ]+gRPacketSize[ch]*which );
				if ( offset > gRPacketSize[ ch ] )
					offset = gRPacketSize[ ch ]; // don't fuck up memory if some strange read happened...
				for ( i = 0; i < offset; i++ )
				{
					pData[ i ] = 0;
				}
			}
			break;
		
		case ( 2 ):
			break;
		
		case ( 1 ):
			// turn the volume back up, the spooler is at the beginning of
			// the good SPU buffer...
			BgmSetVolume( ch, gBgmVolume[ch] );
			break;
		
		default:
			printf( "unknown preload state... fire Matt.\n" );
			break;
	}
	--gBgmPreload[ ch ];
}

int _BgmPlay( int status )
{
	int i, ch, read_size, which;
	int *addr, remain;
	Dbg_Printf( "entering _BgmPlay\n" );
	while ( 1 )
	{
		//-- Wait for playing of buffer to finish
		WaitSema(gSem);

		//-- Which channel is the interrupt from?
		if( (gBgmIntr[0] == 1) && ( ch != 0 ) )  ch = 0;
		else if( (gBgmIntr[1] == 1) && ( ch != 1 ) )  ch = 1;
		else if( gBgmIntr[0] == 1 )  ch = 0;
		else if( gBgmIntr[1] == 1 )  ch = 1;
		else continue;

		gBgmIntr[ch] = 0;

		which = 1 - (sceSdBlockTransStatus( ch, 0 )>>24);

		//--- Stopped due to end of data (no looping)
		if( ( gBgmMode[ ch ] & BGM_MODE_TERMINATE ) != 0 )
		{
			WaitSema( gSem ); // Wait until another interrupt is received
			_BgmStopAndUnload( ch, 0 );
			BgmSetVolumeDirect( ch, 0x0 );
			continue;
		}

		//--- Volume change event
		if ( ( gBgmVolumeSet[ ch ] == 1 ) && ( !gBgmPreload[ ch ] ) )
		{
			BgmSetVolumeDirect( ch, gBgmVolume[ch] );
			gBgmVolumeSet[ch] = 0;
		}

		//--- Convert buffer

		BgmRaw2Spu( ch, which );

		//--- File READ for buffer

		remain = gWave[ ch ].size - gWave[ ch ].pos;
		if ( remain > gRPacketSize[ch] )
		{
			if ( gFileFromCD[ ch ] )
			{
				//--- Not end of data
				unsigned int err;
				sceCdStRead( NUM_SECTORS_PER_READ, ( u_int * )( gBuffRaw[ ch ]+gRPacketSize[ch]*which ), STMBLK, &err );
				if ( err )
				{
					printf( "PCM C Disk error code 0x%08x\n", err );
				}
				read_size = gRPacketSize[ ch ];
			}
			else
			{
				read_size = read (gFd[ch], (unsigned char*)(gBuffRaw[ch]+gRPacketSize[ch]*which), gRPacketSize[ch] );
			}
			if ( read_size < gRPacketSize[ch] )
				continue; //retry
			if ( gBgmPreload[ ch ] )
			{
				PreloadFile( which, ch );
			}
			gWave[ch].pos += read_size;
		}
		else  //--- End of data
		{
			if ( gFileFromCD[ ch ] )
			{
				//--- Not end of data
				unsigned int err;
				sceCdStRead( NUM_SECTORS_PER_READ, ( u_int * )( gBuffRaw[ ch ]+gRPacketSize[ch]*which ), STMBLK, &err );
				if ( err )
				{
					printf( "PCM C Disk error code 0x%08x\n", err );
				}
				read_size = gRPacketSize[ ch ];
			}
			else
			{
				read_size = read (gFd[ch], (unsigned char*)(gBuffRaw[ch]+gRPacketSize[ch]*which), remain );
			}
			
			if( read_size < remain ) continue; //retry
	    	
			PRINTF(("end of PCM track - ch %d\n", ch));

			addr = ( int * )( gBuffRaw[ ch ] + ( gRPacketSize[ ch ] * which ) + remain );
			for( i = 0; i < ((gRPacketSize[ch]-remain)>>2); i++ )
			{
				*(addr++) = 0;
			}
			gBgmMode[ch] &= BGM_MASK_STATUS;
			gBgmMode[ch] |= BGM_MODE_TERMINATE;
		}

		//-- Stop event
		if( (gBgmPause[ch] == 1) )
		{
			//printf( "_BgmPlay :: pause\n" );
			_BgmPause( ch, 0 );
			BgmSetVolumeDirect(ch, 0x0);
			gBgmPause[ch] = 0;
		}
	}
	return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */

// Ha ha I added some stuff... FUCK YOU SONY!
