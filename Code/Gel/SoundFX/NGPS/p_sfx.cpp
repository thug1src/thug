//--------------------------------------------------------------------------------------------
// Playstation 2 sound effect functionality...
// Taken from samples on Sony website, modified by mjd.
//--------------------------------------------------------------------------------------------

#include <eetypes.h>
#include <eekernel.h>
#include <stdio.h>
#include <sif.h>
#include <sifdev.h>
#include <sdrcmd.h>
#include <libsdr.h>
#include <sdmacro.h>
#include <string.h>

#include <core/macros.h>
#include <core/defines.h>
#include <gel/soundfx/soundfx.h>
#include <gel/soundfx/ngps/p_sfx.h>
#include <gel/music/ngps/pcm/pcm.h>
#include <gel/music/ngps/p_music.h>
#include <gel/music/music.h>
#include <sys/file/filesys.h>

#include <gel/music/ngps/pcm/pcm.h>

#define NO_SOUND	0

namespace Pcm
{
extern int	gNonAllignedIopBuffer;
};

namespace Sfx
{



// Macros and constants:

// This array maps the platform-independent reverb modes specified in reverb.q to the available Ps2 reverb modes.
int ReverbModes[] =
{
	// put the default first in this list, since the default param to SetReverb() is 0
	SD_REV_MODE_HALL,			// REV_MODE_DEFAULT
	SD_REV_MODE_HALL,			// REV_MODE_GENERIC
	SD_REV_MODE_ROOM,			// REV_MODE_PADDEDCELL
	SD_REV_MODE_ROOM,			// REV_MODE_ROOM
	SD_REV_MODE_STUDIO_A,		// REV_MODE_BATHROOM
	SD_REV_MODE_ROOM,			// REV_MODE_LIVINGROOM
	SD_REV_MODE_STUDIO_C,		// REV_MODE_STONEROOM
	SD_REV_MODE_HALL,			// REV_MODE_AUDITORIUM
	SD_REV_MODE_HALL,			// REV_MODE_CONCERTHALL
	SD_REV_MODE_ROOM,			// REV_MODE_CAVE
	SD_REV_MODE_HALL,			// REV_MODE_ARENA
	SD_REV_MODE_HALL,			// REV_MODE_HANGAR
	SD_REV_MODE_HALL,			// REV_MODE_CARPETEDHALLWAY
	SD_REV_MODE_HALL,			// REV_MODE_HALLWAY
	SD_REV_MODE_HALL,			// REV_MODE_STONECORRIDOR
	SD_REV_MODE_HALL,			// REV_MODE_ALLEY
	SD_REV_MODE_HALL,			// REV_MODE_FOREST
	SD_REV_MODE_HALL,			// REV_MODE_CITY
	SD_REV_MODE_HALL,			// REV_MODE_MOUNTAINS
	SD_REV_MODE_HALL,			// REV_MODE_QUARRY
	SD_REV_MODE_HALL,			// REV_MODE_PLAIN
	SD_REV_MODE_HALL,			// REV_MODE_PARKINGLOT
	SD_REV_MODE_PIPE,			// REV_MODE_SEWERPIPE
	SD_REV_MODE_PIPE,			// REV_MODE_UNDERWATER
};

#define NUM_REVERB_MODES ( ( int )( sizeof( ReverbModes ) / sizeof( int ) ) )

#if NUM_STREAMS > 3
#error tell matt to handle more than 3 streams in sfx.cpp
#endif

#if NUM_STREAMS == 3
#define VOICES_MINUS_STREAMS ( 0xFFFFFF & ~( ( 1 << STREAM_VOICE( 0 ) ) | ( 1 << STREAM_VOICE( 1 ) ) | ( 1 << STREAM_VOICE( 2 ) ) ) )
#else
#define VOICES_MINUS_STREAMS ( 0xFFFFFF & ~( ( 1 << MUSIC_L_VOICE ) | ( 1 << MUSIC_R_VOICE ) ) )
#endif
#define VOICES_MINUS_NON_REVERB ( ( ( 1 << NUM_NO_REVERB_VOICES ) - 1 ) << NUM_NO_REVERB_START_VOICE )

// convert sample rate to pitch setting
#define FREQ2PITCH( x ) ( ( ( float ) ( ( x ) * 100 ) / 48000.0f ) )

#define DMA_CH						0

#if ( ( CORE_0_EFFECTS_END_ADDRESS - CORE_0_EFFECTS_START_ADDRESS ) < ( RAM_NEEDED_FOR_EFFECTS ) )
#error "not enough room for effects"
#endif

int sTargetReverbDepth = 0;
int sCurrentReverbDepth = 0;

// allocated SPU RAM
int SpuRAMUsedTemp = 0;
int SpuRAMUsedPerm = 0;

int LoopingSounds[ NUM_CORES ];
// gotta keep track of voices used, to be able
// to tell whether a voice is free or not...
volatile int VoicesUsed[ NUM_CORES ];
volatile int VoicesCleared[ NUM_CORES ];

// avoid checking voice status more than once per frame:
//uint64	gLastVoiceAvailableUpdate[ NUM_CORES ];
//int		gAvailableVoices[ NUM_CORES ];
float	gSfxVolume = 100.0f;

#define MAX_VOLUME					0x3FFF  // max playstation 2 volume...
#define MAX_REVERB_DEPTH			0x3FFF
#define MAX_PITCH					0x3FFF
#define UNALTERED_PITCH				4096

#define BLOCKHEADER_LOOP_BIT	( 1 << 9 )

// Internal function prototypes:

float 	GetPitchValue( VagHeader *VagHdr );
int 	convertEnd( int inp );
bool	PS2Sfx_LoadVAG( PlatformWaveInfo *pInfo, const char *filename, bool loadPerm, float *samplePitch = NULL );
int		GetCore( int whichVoice );

int		PS2Sfx_PlaySound( PlatformWaveInfo *pInfo, int voiceNumber, float volL = 100.0f, float volR = 100.0f, float pitch = 100.0f );
void	PS2Sfx_StopSound( int voiceNumber );
int		PS2Sfx_GetVoiceStatus( int core );

#define ALLIGN_REQUIREMENT	64 // EE to IOP
#define SIZE_OF_LEADIN		16
#define USE_CLEVER_TRICK_FOR_CLOBBERED_DATA		1

// IOP_MIN_TRANSFER must be a power of 2!!!
#define IOP_MIN_TRANSFER	64	// iop will clobber memory if you're writing above a sound.  we'll restore the
								// potentially clobbered memory by keeping a buffer containing the beginning of
								// the previously loaded perm sound, and copying that to the end of our new
								// data...

#if USE_CLEVER_TRICK_FOR_CLOBBERED_DATA
#define		PAD_TO_PREVENT_CLOBBERING		0
#else
#define		PAD_TO_PREVENT_CLOBBERING		64
#endif

#define FULL_NAME_SIZE		256

int GetMemAvailable( void )
{
	
	return ( MAX_SPU_RAM_AVAILABLE - ( SpuRAMUsedTemp + SpuRAMUsedPerm ) );
}

//--------------------------------------------------------------------------------------------
// Function: int EELoadVAG(char *filename ); //, int *samplePitch)
// Description: Transfers specified filename into SPU2 RAM and returns an address value for
//		for use in the SetVoice() function
// Parameters:  filename to be transfered
//
// Returns: value referencing the vag address in SPU2 RAM
// Notes: This transfers all data in a blocking fashion, but quickly.
//-------------------------------------------------------------------------------------------
bool PS2Sfx_LoadVAG( PlatformWaveInfo *pInfo, const char *filename, bool loadPerm, float *samplePitch)
{
	
	
	uint32 size = 0;				// file size
	int vagAddr = 0;			// VAG address in SPU2 RAM, returned to calling function
	VagHeader vagHeader;
	char fullname[ FULL_NAME_SIZE ]; // sound names can be HUGE...
	unsigned int IOPbuffer = 0; // IOP memory buffer
	
	Dbg_MsgAssert( strlen( filename ) < FULL_NAME_SIZE - 30,( "Increase buffer for full filename..." ));
	sprintf(fullname, "sounds\\vag\\%s.vag", filename);
	//Matt("Loading sound %s\n", fullname);
    
	void *fp = File::Open( fullname, "rb" );
	if ( !fp )
	{
		Dbg_MsgAssert( 0,( "LoadSound: Couldn't find file %s", filename ));
		printf( "load sound failed to find %s\n", filename );
		return ( false );
	}
	File::Seek( fp, 0, SEEK_SET );
	if ( SIZE_OF_VAGHEADER != File::Read( &vagHeader, 1, SIZE_OF_VAGHEADER, fp ) )
	{
		// error reading header...
		Dbg_MsgAssert( 0,( "Sorry, young lady, but your VAG is too small" ));
//		Dbg_MsgAssert( 0,( "Not enough bytes to satisfy our VAG" ));
	}
	/*  this fucked things up...
	int endType = 0;
	if ( strncmp( vagHeader.ID, "VAGp", 4 ) == 0 )
		endType = 0;
	else
		endType = 1;
	
	size = ( ( endType == 0 ) ? convertEnd( vagHeader.dataSize ) : vagHeader.dataSize );
	*/
	size = convertEnd( vagHeader.dataSize );

	Dbg_MsgAssert( size > 0,( "There's nothing worse than an empty VAG." ));

	//Dbg_Message( "vag header size %d data size %d", SIZE_OF_VAGHEADER, size );

#if USE_CLEVER_TRICK_FOR_CLOBBERED_DATA
	uint8 *buffer = new uint8[ size + ALLIGN_REQUIREMENT + IOP_MIN_TRANSFER ];
#else
	uint8 *buffer = new uint8[ size + ALLIGN_REQUIREMENT ];
#endif
	File::Seek( fp, SIZE_OF_VAGHEADER, SEEK_SET );
	
	uint8 *allignedBuffer = ( uint8 * )( ( ( int ) buffer + ( ALLIGN_REQUIREMENT - 1 ) ) & ~( ALLIGN_REQUIREMENT - 1 ) );
	
	uint32 readsize = File::Read( allignedBuffer, 1, size, fp );
	if ( size !=  readsize)
	{
		Dbg_Message( "if non-converted size %d is the correct size of file %s, inconsistent VAG format", vagHeader.dataSize + SIZE_OF_VAGHEADER, filename );
		Dbg_MsgAssert( 0,( "VAG %s is %d bytes, not as big as she said it would be (%d bytes)", filename, readsize, size ));
	}
	File::Close( fp );
		
	if ( samplePitch )
	{
		*samplePitch = GetPitchValue( &vagHeader );
	}
//	Dbg_Message( "Pitch value %f", GetPitchValue( &vagHeader ) );

	// make sure we've got room for this sound in RAM:
	// sound data must be 16byte aligned in the SPU, so check for that
	// also, the IOP seems to transfer only in 128 byte chunks, so let's
	// pad the buffer if we're writing a permanent sound (which stacks from
	// the bottom if RAM up) and write the potentially clobbered bytes from
	// the previously loaded permanent sound into the end of the buffer to
	// be transfered (clever huh?)  NOTE: This didn't seem to work without
	// flushing the cache...  Gives everything in memory time to settle I
	// guess?
	
	if ( loadPerm )
	{
#if USE_CLEVER_TRICK_FOR_CLOBBERED_DATA
		int i;
		int overshoot = 0; // need to initialize to zero!
		static uint8 prevPermBuffer[ IOP_MIN_TRANSFER ];
		static int prevVagAddr;
        if ( !SpuRAMUsedPerm )
		{
			// don't store nuthin' first time through...
			prevVagAddr = 0;
		}
#endif
		// stack the permanent sounds up from the bottom:
		SpuRAMUsedPerm += ( size + PAD_TO_PREVENT_CLOBBERING );
		if ( ( END_WAVE_DATA_ADDR - SpuRAMUsedPerm ) & 0x0F )
		{
#ifdef __NOPT_ASSERT__
			int oldSpuRAM = SpuRAMUsedPerm;
#endif			
			SpuRAMUsedPerm += ( ( END_WAVE_DATA_ADDR - SpuRAMUsedPerm ) & 0x0F );
			Dbg_MsgAssert( SpuRAMUsedPerm > oldSpuRAM,( "Duh asshole." ));
		}
		vagAddr = END_WAVE_DATA_ADDR - SpuRAMUsedPerm;
		Dbg_MsgAssert( !( vagAddr & 0x0F ),( "Fire Matt please." ));
		
#if USE_CLEVER_TRICK_FOR_CLOBBERED_DATA
		if ( prevVagAddr )
		{
			// don't have to do this if this is the first loaded perm sound...
			// hence condition above...
			
			// find out how much of the previous perm data we'd be writing
			// over, add that to the end of our buffer...
			if ( size & ( IOP_MIN_TRANSFER - 1 ) )
			{
//				Dbg_Message( "size %d over 64-byte boundary", size & ( IOP_MIN_TRANSFER - 1 ) );
				overshoot = ( IOP_MIN_TRANSFER ) - ( size & ( IOP_MIN_TRANSFER - 1 ) );
//				Dbg_Message( "overshoot %d", overshoot );
				int spaceBetween = prevVagAddr - ( vagAddr + size );
//				Dbg_Message( "space between %d", spaceBetween );
				int numBytesClobbered = overshoot - spaceBetween;
				Dbg_MsgAssert( numBytesClobbered <= IOP_MIN_TRANSFER,( "how in the world?" ));
//				Dbg_Message( "numBytesClobbered = %d", numBytesClobbered );
				for ( i = 0; i < numBytesClobbered; i++ )
				{
					// store the previous buffer at the end of our buffer,
					allignedBuffer[ size + i + spaceBetween ] = prevPermBuffer[ i ];
				}
			}
			else
			{
				overshoot = 0;
			}	
			size += overshoot;
			// add on the size of the overshoot, which in the case of permanent sounds
			// has data from a previous sound (which will be clobbered), written in.
//			size += overshoot;
			Dbg_MsgAssert( !( size & ( IOP_MIN_TRANSFER - 1 ) ),( "Fire Matt." ));
			
		}
		for ( i = 0; i < IOP_MIN_TRANSFER; i++ )
		{
			// store the beginning of our new buffer into the static buffer
			// for when the next perm sound loads...
			prevPermBuffer[ i ] = allignedBuffer[ i ];
		}
		prevVagAddr = vagAddr;
#endif		
	} // if ( loadPerm )
	else
	{
		if ( ( SpuRAMUsedTemp + BASE_WAVE_DATA_ADDR ) & 0x0F )
		{
			SpuRAMUsedTemp += 0x10 - ( ( SpuRAMUsedTemp + BASE_WAVE_DATA_ADDR ) & 0x0F );
		}
		vagAddr = BASE_WAVE_DATA_ADDR + SpuRAMUsedTemp;
		SpuRAMUsedTemp += ( size );
	}
	
	// transfer buffer to IOP, it's a DMA operation so FlushCache() to ensure cache is flushed
	FlushCache( 0 );

	// Ken: Instead of borrowing a new chunk of memory from the IOP, (which causes it to often
	// run out cos we're so tight on memory) use the already existing
	// gNonAllignedIopBuffer, used by streams and music etc. They have no need of it at this point.
	//IOPbuffer = ( unsigned int )sceSifAllocIopHeap( size );
	Dbg_MsgAssert(size<TOTAL_IOP_BUFFER_SIZE_NEEDED + ALLIGN_REQUIREMENT,("Oops! Vag '%s' too big for iop buffer",filename));
	Dbg_MsgAssert(Pcm::gNonAllignedIopBuffer,("NULL gNonAllignedIopBuffer"));
	IOPbuffer=Pcm::gNonAllignedIopBuffer;
	
	Dbg_MsgAssert( !( IOPbuffer & 3 ),( "This address should be four byte alligned." ));
	if ( !IOPbuffer )
	{
		Dbg_MsgAssert( 0,( "Failed to allocate IOP memory\n" ));
		return false;
	}
	
	if ( sceSdTransToIOP( allignedBuffer, IOPbuffer, size, 1 ) == -1 )
	{
		Dbg_MsgAssert( 0,( "Failed to transfer to IOP\n" ));
		return false;
	}
	// now transfer from IOP memory into SPU2 RAM
	
	if ( ( SpuRAMUsedTemp + SpuRAMUsedPerm + IOP_MIN_TRANSFER ) > MAX_SPU_RAM_AVAILABLE )
	{
		Dbg_MsgAssert( 0,( "Our VAG space has been RAMed to the hilt" ));
	}

	if (!CSpuManager::sVoiceTrans(DMA_CH, SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA, (void *) IOPbuffer, (uint32 *) vagAddr, size))
	{
		Dbg_MsgAssert( 0,( "Failed to Transfer to SPU2 size %d vagAddr %x IOPBuffer addr %x\n", size, vagAddr, IOPbuffer ));
	}

	// Ken: Commented this out because now we're using Pcm::gNonAllignedIopBuffer instead.
	// Now free up IOP and EE memory, check this return value is zero, as documentation (iopservice.txt v1.5) seems to be wrong
	//if( sceSifFreeIopHeap( (void * ) IOPbuffer ) != 0 )
	//{
	//	Dbg_MsgAssert( 0,( "Failed to free IOP memory\n" ));
	//}

	pInfo->vagAddress = vagAddr;
	uint16 blockHeader = *( ( uint16 * ) allignedBuffer + SIZE_OF_LEADIN );
	pInfo->looping = ( blockHeader & BLOCKHEADER_LOOP_BIT ) ? true : false;
	
	delete buffer;
	return ( true );
}

//--------------------------------------------------------------------------------------------
// Function    : float GetPitchValue(char *buffer)
// Description : gets pitch value from VAG header
//
// Parameters  : pointer to VAG header
//
// Returns     : pitch value
// Notes       : 
//-------------------------------------------------------------------------------------------
float GetPitchValue( VagHeader *VagHdr )
{
	
/*	int endType = 0;

	if ( strncmp( VagHdr->ID, "VAGp", 4 ) == 0 )
		endType = 0;
	else
		endType = 1;

	if(endType == 0)
    {*/
		return ( ( FREQ2PITCH( convertEnd( VagHdr->sampleFreq ) ) ) );
/*	}
	else
	{
		return ( FREQ2PITCH( VagHdr->sampleFreq ) );
	}
	return 0;*/
}

#ifdef __USER_MATT__
//--------------------------------------------------------------------------------------------
// Function    : void PrintVAGDetails(char *buffer)
// Description : debug function to print out VAG details
//
// Parameters  : pointer to VAG header
//
// Returns     : 
// Notes       : 
//-------------------------------------------------------------------------------------------
void PrintVAGDetails(char *buffer)
{
	
	VagHeader *VagHdr;
	int i;
	int endType = 0;	// 0 for PC, 1 for MAC

	VagHdr = (VagHeader *)buffer;

	Dbg_Message("Checking for VAGp string ..'");
	for(i=0;i<4;i++)
	{
		Dbg_Message("%c",VagHdr->ID[i]);
	}
	Dbg_Message("' found.\n");

	Dbg_Message("VAG Details\n");
	Dbg_Message("===========\n");

	// check for either little or big endian format (PC or MAC)
	if(strncmp(VagHdr->ID,"VAGp",4) == 0)
		endType = 0;
	else
		endType = 1;

	Dbg_Message("ID         : ");
	for(i=0;i<4;i++)
	{
		Dbg_Message("%c",VagHdr->ID[i]);
	}
	Dbg_Message("\n");

	Dbg_Message("VERSION    : ");
	Dbg_Message("0x%x", (endType == 0) ? convertEnd(VagHdr->version) : VagHdr->version);
	Dbg_Message("\n");

	Dbg_Message("RESERVED   : ");
	for(i=0;i<4;i++)
	{
		Dbg_Message("%x",VagHdr->reserved1[i]);
	}
	Dbg_Message("\n");
		
	Dbg_Message("DATASIZE   : ");
	Dbg_Message("0x%x", (endType == 0) ? convertEnd(VagHdr->dataSize) : VagHdr->dataSize);
	Dbg_Message("\n");

	Dbg_Message("SAMPLEFREQ : ");
	Dbg_Message("0x%x (%d)", (endType == 0) ? convertEnd(VagHdr->sampleFreq) : VagHdr->sampleFreq, (endType == 0) ? convertEnd(VagHdr->sampleFreq) : VagHdr->sampleFreq);
	Dbg_Message("\n");

	Dbg_Message("RESERVED   : ");
	for(i=0;i<12;i++)
	{
		Dbg_Message("%x",VagHdr->reserved2[i]);
	}
	Dbg_Message("\n");

	Dbg_Message("NAME       : ");
	for(i=0;i<16;i++)
	{
		if(VagHdr->name[i] == '\0')
			break;
		Dbg_Message("%c",VagHdr->name[i]);
	}
	Dbg_Message("\n");

	return;
}
#endif

//--------------------------------------------------------------------------------------------
// Function    : int convertEnd(int inp)
// Description : needed for swopping endian values if VAG has been created by PC
//
// Parameters  : 
//
// Returns     : 
// Notes       : 
//-------------------------------------------------------------------------------------------
int convertEnd(int inp)
{
	
	// swap bytes 0 and 4, 1 and 2
	return((inp & 0xFF) << 24) | ((inp & 0xFF00) << 8) | ((inp & 0xFF0000) >> 8) | ((inp & 0xFF000000) >> 24);
}

// Master volumeLevel from 0 to 100 percent...
void SetVolumePlease( float volumeLevel )
{
	
	gSfxVolume = volumeLevel;

	Pcm::PCMAudio_SetStreamGlobalVolume( ( unsigned int ) volumeLevel );
/*
	int vol = ( int )PERCENT( volumeLevel, MAX_VOLUME );
	if ( vol > MAX_VOLUME )
		vol = MAX_VOLUME;
	
	Dbg_MsgAssert( vol >= 0.0f,( "Can't have negative main volume." ));

	int i;
	// set master volume for both cores, both blocking
	for ( i = 0; i < 2; i++ )
	{
		
		if ( !CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_MVOLL, vol ) )
		{
			Dbg_MsgAssert( 0,( "Error Setting Left Vol on core %d\n",i));
		}
		if ( !CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_MVOLR, vol ) )
		{
			Dbg_MsgAssert( 0,( "Error Setting Right Vol on core %d\n",i));
		}
	}*/
} // end of SetVolumePlease( )

void ReverbOff( void )
{
	
	int i;
	sceSdEffectAttr r_attr;

	sTargetReverbDepth = sCurrentReverbDepth = 0;
	// --- set reverb attribute
	r_attr.depth_L  = 0;
	r_attr.depth_R  = 0;
	r_attr.delay    = 0;
	r_attr.feedback = 0;
	r_attr.mode = SD_REV_MODE_OFF;
#if REVERB_ONLY_ON_CORE_0
	i = 0;  // just set reverb on core 0 (not the music core or any sfx on core 1)
#else
	for ( i = 0; i < 2; i++ )
#endif
	{
		// Shut volume first
		if(!CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_EVOLL , sTargetReverbDepth ) )
			Dbg_Message("Error setting reverb2\n");
		if(!CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_EVOLR , sTargetReverbDepth ) )
			Dbg_Message("Error setting reverb3\n");

		// just turning off reverb on all voices...
		if(!CSpuManager::sSendSdRemote( rSdSetSwitch, i | SD_S_VMIXEL , 0 ) )
			Dbg_Message("Error setting reverb4\n");
		if(!CSpuManager::sSendSdRemote( rSdSetSwitch, i | SD_S_VMIXER , 0 ) )
			Dbg_Message("Error setting reverb5\n");
		if( !CSpuManager::sSendSdRemote( rSdSetEffectAttr, i, (uint32) &r_attr ) )
			Dbg_Message( "Error setting reverb\n" );
	}
}

#define REVERB_STEP ( ( MAX_REVERB_DEPTH / 20 ) + 1 )

void DoReverbFade( void )
{
	int i;
	if ( sTargetReverbDepth != sCurrentReverbDepth )
	{
		if ( sCurrentReverbDepth < sTargetReverbDepth )
		{
			sCurrentReverbDepth += REVERB_STEP;
			if ( sCurrentReverbDepth > sTargetReverbDepth )
			{
				sCurrentReverbDepth = sTargetReverbDepth;
			}
		}
		else
		{
			sCurrentReverbDepth -= REVERB_STEP;
			if ( sCurrentReverbDepth < sTargetReverbDepth )
			{
				sCurrentReverbDepth = sTargetReverbDepth;
				if ( !sTargetReverbDepth )
				{
					ReverbOff( );
				}
			}
		}
		
#if REVERB_ONLY_ON_CORE_0
		i = 0;  // just set reverb on core 0 (not the music core or any sfx on core 1)
#else
		for ( i = 0; i < 2; i++ )
#endif
		{
			if(!CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_EVOLL , sCurrentReverbDepth ) )
				Dbg_Message("Error setting reverb2\n");
			
			if(!CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_EVOLR , sCurrentReverbDepth ) )
				Dbg_Message("Error setting reverb3\n");
		}
	}
}

void InitReverbAddr( void )
{
#if REVERB_ONLY_ON_CORE_0
	if ( !CSpuManager::sSendSdRemote( rSdSetAddr, 0 | SD_A_ESA, CORE_0_EFFECTS_START_ADDRESS ) )
		Dbg_Message( "Error setting reverb\n" );
	if ( !CSpuManager::sSendSdRemote( rSdSetAddr, 0 | SD_A_EEA, CORE_0_EFFECTS_END_ADDRESS ) )
		Dbg_Message( "Error setting reverb\n" );
#else
	if ( !CSpuManager::sSendSdRemote( rSdSetAddr, 0 | SD_A_ESA, CORE_0_EFFECTS_START_ADDRESS ) )
		Dbg_Message( "Error setting reverb\n" );
	if ( !CSpuManager::sSendSdRemote( rSdSetAddr, 0 | SD_A_EEA, CORE_0_EFFECTS_END_ADDRESS ) )
		Dbg_Message( "Error setting reverb\n" );
		
	if ( !CSpuManager::sSendSdRemote( rSdSetAddr, 1 | SD_A_ESA, CORE_1_EFFECTS_START_ADDRESS ) )
		Dbg_Message( "Error setting reverb\n" );
	if ( !CSpuManager::sSendSdRemote( rSdSetAddr, 1 | SD_A_EEA, CORE_1_EFFECTS_END_ADDRESS ) )
		Dbg_Message( "Error setting reverb\n" );
#endif
}

//--------------------------------------------------------------------------------------------
// Function: void SetReverb(void)
// Description: Sets reverb level and type
// Parameters:  none
//
// Returns:
//-------------------------------------------------------------------------------------------
void SetReverbPlease( float reverbLevel, int reverbType, bool instant )
{
	
	int i;
	sceSdEffectAttr r_attr;

	Dbg_MsgAssert( reverbType >= 0,( "Bad reverb mode." ));
	Dbg_MsgAssert( reverbType < NUM_REVERB_MODES,( "Bad reverb mode..." ));
	
	if ( !reverbLevel )
	{
		sTargetReverbDepth = 0;
		if ( instant )
		{
			ReverbOff( );
		}
	}
	else
	{
		// Calling this in case it wasn't set before.  There is a bug with the SPU reg set code where some
		// messages are lost
		InitReverbAddr();

		sTargetReverbDepth = ( int )PERCENT( MAX_REVERB_DEPTH, reverbLevel );
		
		// --- set reverb attribute
		r_attr.depth_L  = 0x3fff;
		r_attr.depth_R  = 0x3fff;
		r_attr.delay    = 30;
		r_attr.feedback = 200;
		r_attr.mode = ReverbModes[ reverbType ];
#if REVERB_ONLY_ON_CORE_0
		i = 0;
#else		
		for( i = 0; i < 2; i++ )
#endif
		{
			if( !CSpuManager::sSendSdRemote( rSdSetEffectAttr, i, (uint32) &r_attr ) )
				Dbg_Message( "Error setting reverb\n" );
	
			if(!CSpuManager::sSendSdRemote( rSdSetCoreAttr, i | SD_C_EFFECT_ENABLE, 1 ) )
				Dbg_Message("Error setting reverb\n");
			if ( instant )
			{
				if(!CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_EVOLL , sTargetReverbDepth ) )
					Dbg_Message("Error setting reverb2\n");
				if(!CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_EVOLR , sTargetReverbDepth ) )
					Dbg_Message("Error setting reverb3\n");
				sCurrentReverbDepth = sTargetReverbDepth;
			}

			uint32 reverb_voices = (i == NUM_NO_REVERB_CORE) ? VOICES_MINUS_NON_REVERB : VOICES_MINUS_STREAMS;
			if(!CSpuManager::sSendSdRemote( rSdSetSwitch, i | SD_S_VMIXEL , reverb_voices ) )
				Dbg_Message("Error setting reverb4\n");
			if(!CSpuManager::sSendSdRemote( rSdSetSwitch, i | SD_S_VMIXER , reverb_voices ) )
				Dbg_Message("Error setting reverb5\n");
			
		}
	}
} // end of SetReverb( )

//--------------------------------------------------------------------------------------------
// Function    : void StopSound(int voiceNumber)
// Description : turns off voice
// Parameters  : voiceNumber - voice to stop
//
// Returns:
// Notes: 
//-------------------------------------------------------------------------------------------
void PS2Sfx_StopSound(int voiceNumber)
{
	//Dbg_Message("Turning off voice number %d", voiceNumber);

	int coreUsed;

	if ( voiceNumber >= NUM_VOICES_PER_CORE )
	{
		voiceNumber -= NUM_VOICES_PER_CORE;
		coreUsed = SD_CORE_1;
	}
	else
	{
		coreUsed = SD_CORE_0;
	}

	if ( !CSpuManager::sSendSdRemote( rSdSetSwitch, coreUsed | SD_S_KOFF , 1 << voiceNumber ) )
		Dbg_MsgAssert( 0,( "Failed to turn off\n" ));

	// turn off looping sound flag (if it's even on...)
	// Garrett: Took out this line so that we don't possibly turn off and on a voice in the same SPU tick.
	//LoopingSounds[ coreUsed ] &= ~( 1 << voiceNumber );
//	SoundsThatWereSwitchedOff[ coreUsed ] |= ( 1 << voiceNumber );
	// might not want this... can't write too often to a voice...
	// Garrett: Took out this line so that we don't possibly turn off and on a voice in the same SPU tick.
	// The only bad thing is this won't be cleared until the next frame.
	//VoicesUsed[ coreUsed ] &= ~( 1 << voiceNumber );
	VoicesCleared[ coreUsed ] |= 1 << voiceNumber;
	return;
}

bool LoadSoundPlease( const char *sfxName, uint32 checksum, PlatformWaveInfo *pInfo, bool loadPerm )
{
	
	Dbg_MsgAssert( pInfo,( "Null pointer to PlatformWaveInfo." ));
	return PS2Sfx_LoadVAG( pInfo, sfxName, loadPerm, &pInfo->pitchAdjustmentForSamplingRate );
} // end of LoadSoundPlease( )

int	GetCore( int voiceNumber )
{
	
	if ( voiceNumber >= NUM_VOICES_PER_CORE )
	{
		voiceNumber -= NUM_VOICES_PER_CORE;
		Dbg_MsgAssert( voiceNumber < NUM_VOICES_PER_CORE,( "Bad voice number." ));
		return ( SD_CORE_1 );
	}
	Dbg_MsgAssert( voiceNumber >= 0,( "Bad voice number." ));
	return ( SD_CORE_0 );
	
} // end of GetCore( )

//void SetVoiceParameters( int voiceNumber, float volL, float volR, float fPitch )
void SetVoiceParameters( int voiceNumber, sVolume *p_vol, float fPitch )
{
	int coreUsed = GetCore( voiceNumber );

	if ( coreUsed == SD_CORE_1 )
	{
		voiceNumber -= NUM_VOICES_PER_CORE;
	}
	Dbg_MsgAssert( voiceNumber < NUM_VOICES_PER_CORE,( "What in tarnation?  Voice out of range." ));

	// adjust for SFX Volume level:
	p_vol->PercentageAdjustment( gSfxVolume );

	int lVol = (int)PERCENT( MAX_VOLUME, p_vol->GetChannelVolume( 0 ));	
	int rVol = (int)PERCENT( MAX_VOLUME, p_vol->GetChannelVolume( 1 ));	

	if ( lVol > MAX_VOLUME )
		lVol = MAX_VOLUME;
	else if ( lVol < -MAX_VOLUME )
		lVol = -MAX_VOLUME;

	if ( rVol > MAX_VOLUME )
		rVol = MAX_VOLUME;
	else if ( rVol < -MAX_VOLUME )
		rVol = -MAX_VOLUME;

	if ( fPitch )
	{
		int pitch = ( int )PERCENT( UNALTERED_PITCH, fPitch );
		if ( pitch > MAX_PITCH )
			pitch = MAX_PITCH;
		if ( pitch <= 0 )
			pitch = 1;
		if ( !CSpuManager::sSendSdRemote( rSdSetParam, coreUsed | ( voiceNumber << 1 ) | SD_VP_PITCH, pitch ) )
			Dbg_Message("Failed pitch setting\n");
	}

	// for phasing effects (of sounds behind you), set the volume
	// to 0x7fff (left channel will be negative if sound is behind
	// us)
	if ( lVol < 0 )
	{
		lVol = 0x7fff + lVol;
	}
	if ( rVol < 0 )		// Just in case we start phase shifting the right side instead
	{
		rVol = 0x7fff + rVol;
	}
	if ( !CSpuManager::sSendSdRemote( rSdSetParam, coreUsed | ( voiceNumber << 1 ) | SD_VP_VOLL, lVol ) )
		Dbg_Message( "Error1\n" );
	if ( !CSpuManager::sSendSdRemote( rSdSetParam, coreUsed | ( voiceNumber << 1 ) | SD_VP_VOLR, rVol ) )
		Dbg_Message("Error2\n");
} // end of SetVoiceParameters( )

int PlaySoundPlease( PlatformWaveInfo *pInfo, sVolume *p_vol, float pitch, bool no_reverb )
{
	#if NO_SOUND
	return ( -1 );
	#endif

	int voiceAvailableFlags;
	int i;

	if ( no_reverb )
	{
		voiceAvailableFlags = PS2Sfx_GetVoiceStatus( NUM_NO_REVERB_CORE );

		for ( i = NUM_NO_REVERB_START_VOICE; i < ( NUM_NO_REVERB_VOICES + NUM_NO_REVERB_START_VOICE ); i++ )
		{
			if ( voiceAvailableFlags & ( 1 << i ) )
			{
				return ( PS2Sfx_PlaySound( pInfo, i + (NUM_NO_REVERB_CORE * NUM_VOICES_PER_CORE), p_vol->GetChannelVolume( 0 ), p_vol->GetChannelVolume( 1 ), pitch ));
			}
		}

//		Dbg_MsgAssert(0, ("Ran out of NO_REVERB voices"));
		printf("WARNING: Ran out of NO_REVERB voices - using reverb voices\n");
	}

	voiceAvailableFlags = PS2Sfx_GetVoiceStatus( 0 );
	for ( i = 0; i < NUM_VOICES_PER_CORE; i++ )
	{
		if ( voiceAvailableFlags & ( 1 << i ) )
		{
			//gAvailableVoices[ 0 ] &= ~( 1 << i );
			return ( PS2Sfx_PlaySound( pInfo, i, p_vol->GetChannelVolume( 0 ), p_vol->GetChannelVolume( 1 ), pitch ));
		}
	}
	
	// 2nd core ( 1st core busy, all voices used )
	// For now, allow reverb sounds to use non-reverb voices.  They are at the end, anyway
	voiceAvailableFlags = PS2Sfx_GetVoiceStatus( 1 );
	for ( i = 0; i < NUM_VOICES_PER_CORE; i++ )
	{
		if ( voiceAvailableFlags & ( 1 << i ) )
		{
			//gAvailableVoices[ 1 ] &= ~( 1 << i );
			return ( PS2Sfx_PlaySound( pInfo, i + NUM_VOICES_PER_CORE, p_vol->GetChannelVolume( 0 ), p_vol->GetChannelVolume( 1 ), pitch ));
		}
	}

#ifdef	__NOPT_ASSERT__
	CSpuManager::sPrintStatus();
#endif
	Dbg_MsgAssert(0, ("Can't play sound.  Out of voices"));
	return ( -1 );
} // end of PlaySoundPlease( )

void StopSoundPlease( int whichVoice )
{

	
	if ( VoiceIsOn( whichVoice ) )
	{
		PS2Sfx_StopSound( whichVoice );
	}
} // end of StopSoundPlease( )

void PauseSoundsPlease( void )
{
	sVolume silent_volume;
	silent_volume.SetSilent();

	// just turn the volume down on all playing voices...
	for( int i = 0; i < NUM_VOICES; i++ )
	{
		if( VoiceIsOn( i ))
		{
			SetVoiceParameters( i, &silent_volume );
		}
	}
}// end of PauseSounds( )

//--------------------------------------------------------------------------------------------
// Function    : void StopAllSoundFX(void)
// Description : turns off all voices
// Parameters  : 
//
// Returns:
// Notes: 
//-------------------------------------------------------------------------------------------
void StopAllSoundFX( void )
{
	
	int core;
	Pcm::StopMusic( );
	Pcm::StopStreams( );
	for ( core = 0; core < NUM_CORES; core++ )
	{
//		SoundsThatWereSwitchedOff[ core ] = ( ( 1 << NUM_VOICES_PER_CORE ) - 1 );
		VoicesUsed[ core ] = 1;		// So that the KOFF check thinks there are sounds to kill
		VoicesCleared[ core ] = 0;
		LoopingSounds[ core ] = 0;
//		Dbg_Message( "Stopping all sfx -- remind Matt to fix what this is going to do to streams!!!" );
		CSpuManager::sClean();
		CSpuManager::sSendSdRemote( rSdSetSwitch, core | SD_S_KOFF, VOICES_MINUS_STREAMS );
		CSpuManager::sSendSdRemote( rSdSetSwitch, core | SD_S_ENDX, 0 );
		VoicesUsed[ core ] = 0;
	}
	return;
}

//--------------------------------------------------------------------------------------------
// Function    : void PlaySound(int voiceNumber,int pitch,int volume)
// Description : Plays voice
// Parameters  : voiceNumber - voice to play
//		 pitch - pitch to play at
//		 volume - volume to play at
//
// Returns: voice number
//-------------------------------------------------------------------------------------------
int PS2Sfx_PlaySound( PlatformWaveInfo *pInfo, int voiceNumber, float volL, float volR, float pitch )
{
	
	int coreUsed = GetCore( voiceNumber );
	int voiceNumOnCore;

	if ( coreUsed == SD_CORE_1 )
	{
		voiceNumOnCore = voiceNumber - NUM_VOICES_PER_CORE;
	}
	else
		voiceNumOnCore = voiceNumber;

	if ( !CSpuManager::sSendSdRemote( rSdSetAddr, coreUsed | ( voiceNumOnCore << 1 ) | SD_VA_SSA, pInfo->vagAddress ) )
		Dbg_Message( "Error4\n" );
	
	// adjust pitch to account for lower sampling rates:
	pitch = PERCENT( pitch, pInfo->pitchAdjustmentForSamplingRate );
	
	// should be voiceNumber, not voiceNumOnCore:
	sVolume vol;
	vol.SetChannelVolume( 0, volL );
	vol.SetChannelVolume( 1, volR );
	SetVoiceParameters( voiceNumber, &vol, pitch );
	
	if ( !CSpuManager::sSendSdRemote( rSdSetSwitch, coreUsed | SD_S_KON, 0x1 << voiceNumOnCore ) )
		Dbg_Message("Failed setswitch\n");
	
	if ( pInfo->looping )
	{
		LoopingSounds[ coreUsed ] |= ( 1 << voiceNumOnCore );
		//Dbg_Message("Setting looping sound on voice %d core %d", voiceNumOnCore, coreUsed);
		//Dbg_Assert(0);
	}

	// Set the VoicesUsed flag.
	VoicesUsed[ coreUsed ] |= 1 << voiceNumOnCore;
//	printf( "voice %d\n", voiceNumber );
	//Dbg_Message("Voices Used (%x %x) Cleared (%x %x) Looped (%x %x) Voice Status (%x %x)", VoicesUsed[0], VoicesUsed[1], VoicesCleared[0], VoicesCleared[1],
	//																  LoopingSounds[0], LoopingSounds[1], CSpuManager::sGetVoiceStatus(0), CSpuManager::sGetVoiceStatus(1));

	return ( voiceNumber );
} // end of PS2Sfx_PlaySound( )

void CleanUpSoundFX( void )
{
	
	StopAllSoundFX( );
	SpuRAMUsedTemp = 0;
	SetReverbPlease( 0 );
}// end of CleanUpSoundFX( )

#define NUM_VOICES_INCLUDING_STREAMS_PER_CORE 24

//--------------------------------------------------------------------------------------------
// Function    : void InitSound( void )
// Description : initialise sound system
// Parameters  : 
//	
// Returns:
// Notes: 
//--------------------------------------------------------------------------------------------
void InitSoundFX( CSfxManager *p_sfx_manager )
{
	// Set default volume type.
	if( p_sfx_manager )
	{
		p_sfx_manager->SetDefaultVolumeType( VOLUME_TYPE_2_CHANNEL_DOLBYII );
	}
	
	int i, j;
	// set master volume for both cores, both blocking
	for ( i = 0; i < 2; i++ )
	{
		
		if ( !CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_MVOLL, MAX_VOLUME ) )
		{
			Dbg_MsgAssert( 0,( "Error Setting Left Vol on core %d\n",i));
		}
		if ( !CSpuManager::sSendSdRemote( rSdSetParam, i | SD_P_MVOLR, MAX_VOLUME ) )
		{
			Dbg_MsgAssert( 0,( "Error Setting Right Vol on core %d\n",i));
		}
	}
	
//	SetVolumePlease( 100 );
	
	// Sets the reverb address registers
	InitReverbAddr();
			
	CleanUpSoundFX( );

	// Set the envelope info at the end because we sometimes lose messages
	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < NUM_VOICES_INCLUDING_STREAMS_PER_CORE; j++ )
		{
			CSpuManager::sSendSdRemote ( rSdSetParam, i | ( j << 1 ) | SD_VP_ADSR1, 0x00ff );
			CSpuManager::sSendSdRemote ( rSdSetParam, i | ( j << 1 ) | SD_VP_ADSR2, 0x1fc0 );
		}
	}

	return;
}

void PS2Sfx_InitCold( void )
{
	

	CSpuManager::sInit();

//	int i;
//	for ( i = 0; i < NUM_CORES; i++ )
//	{
//		gLastVoiceAvailableUpdate[ i ] = 0;
//		gAvailableVoices[ i ] = 0;
//	}

} // end of PS2Sfx_Init( )

//--------------------------------------------------------------------------------------------
// Function    : int GetVoiceStatus( int core )
// Description : returns bitmask showing which voices are available
// Parameters  : core - SPU core
//	
// Returns     : integer value which is the bitmask for voices
//--------------------------------------------------------------------------------------------
int PS2Sfx_GetVoiceStatus( int core )
{
	
#if 1
	int availableVoices = CSpuManager::sGetVoiceStatus(core);
#else
	int availableVoices;

	if ( gLastVoiceAvailableUpdate[ core ] != Tmr::GetRenderFrame( ) )
	{
		gLastVoiceAvailableUpdate[ core ] = Tmr::GetRenderFrame( );
		CSpuManager::sWaitForIO();
		gAvailableVoices[ core ] = sceSdRemote( 1, rSdGetSwitch, core | SD_S_ENDX );
	}
	// available voices at this point will show all voices that have reached the end...
	availableVoices = gAvailableVoices[ core ];
#endif

	// except we've got to remove looping sounds (they set the ENDX flag first time they reach LOOPEND)
	availableVoices &= ~LoopingSounds[ core ];
	// include the voices that have not ever been used yet (if never used,
	// a voice never has had a chance to set the ENDX register...)
	availableVoices |= ~VoicesUsed[ core ];
	
	return ( availableVoices );
}

bool VoiceIsOn( int whichVoice )
{
	
	int whichCore;
	int voiceAvailableFlags;
		
	if ( whichVoice < NUM_VOICES_PER_CORE )
	{
		whichCore = 0;
	}
	else
	{
		whichCore = 1;
		whichVoice -= NUM_VOICES_PER_CORE;
	}
	
	voiceAvailableFlags = PS2Sfx_GetVoiceStatus( whichCore );
	return ( ( voiceAvailableFlags & ( 1 << whichVoice ) ) ? false : true );
} // end of	VoiceIsOn( )


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PerFrameUpdate( void )
{
	
	DoReverbFade( );
}

/////////////////////////////////////////////////////////////////////////////////
/// CSpuManager

volatile bool					CSpuManager::s_sd_remote_busy;
volatile bool					CSpuManager::s_sd_transferring_voice;
volatile uint32					CSpuManager::s_sd_voice_status[NUM_CORES];
volatile uint32					CSpuManager::s_sd_voice_status_update_frame[NUM_CORES] = { 0, 0 };

uint32							CSpuManager::s_spu_reg_kon[NUM_CORES] = { 0, 0 };
uint32							CSpuManager::s_spu_reg_koff[NUM_CORES] = { 0, 0 };

SSifCmdSoundRequestPacket		CSpuManager::s_sd_remote_commands[CSpuManager::MAX_QUEUED_COMMANDS] __attribute__ ((aligned(16)));
volatile int					CSpuManager::s_sd_remote_command_free_index;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CSpuManager::sInit()
{
	// Init variables
	s_sd_remote_busy = false;
	s_sd_remote_command_free_index = 0;

	if ( sceSdRemoteInit( ) == -1 )
	{
		Dbg_MsgAssert( 0,( "Error Initialising sound\n"));
		return;
	}

	// Initialise IOP side, blocking call
	if ( sceSdRemote( 1, rSdInit, SD_INIT_COLD ) == -1 )
	{
		Dbg_MsgAssert( 0,( "Error Initialising IOP\n"));
		return;
	}

	// Make sure the DMA ID's are 0 so we don't wait for random ID's
	for (int i = 0; i < MAX_QUEUED_COMMANDS; i++)
	{
		s_sd_remote_commands[i].m_request.m_ee_dma_id = 0;
	}

	sceSifAddCmdHandler(SOUND_RESULT_COMMAND, s_result_callback, NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CSpuManager::sClean()
{
	for (int core = 0; core < NUM_CORES; core++)
	{
		s_spu_reg_kon[core] = 0;
		s_spu_reg_koff[core] = 0;
		s_sd_voice_status[core] = 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef	__NOPT_ASSERT__
void			CSpuManager::sPrintStatus()
{
	for (int core = 0; core < NUM_CORES; core++)
	{
		Dbg_Message("Core %d: Voices Used (%x) Cleared (%x) Looped (%x) Voice Status (%x)", core, VoicesUsed[core], VoicesCleared[core],
																		  LoopingSounds[core], sGetVoiceStatus(core));
		Dbg_Message("        Saved KON value (%x) Saved KOFF value (%x)", s_spu_reg_kon[core], s_spu_reg_koff[core]);
	}
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CSpuManager::sWaitForIO()
{
	while (s_sd_remote_busy)
		;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CSpuManager::sUpdateStatus()
{
	//scePrintf("******** Doing CSpuManager::sUpdateStatus() on frame %d\n", Tmr::GetRenderFrame( ));
	for (int core = 0; core < NUM_CORES; core++)
	{
		sSendSdRemote( rSdGetSwitch, core | SD_S_ENDX, 0 );

		// Clear out switches
		//s_spu_reg_kon[core] = 0;
		//s_spu_reg_koff[core] = 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32			CSpuManager::sGetVoiceStatus(int core)
{
	// For now, just go into a wait loop.  In the future, we can probably use the old value
	while (s_sd_voice_status_update_frame[core] != Tmr::GetRenderFrame( ))
		;

	if (s_sd_voice_status_update_frame[core] != Tmr::GetRenderFrame( ))
	{
		Dbg_MsgAssert(0, ("Sound voice status not current: need frame %d but have frame %d", Tmr::GetRenderFrame( ), s_sd_voice_status_update_frame[core]));
	}

	return s_sd_voice_status[core];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CSpuManager::sSendSdRemote(uint16 function, uint16 entry, uint32 value)
{
	return s_queue_new_command(function, entry, value);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CSpuManager::sVoiceTrans(uint16 channel, uint16 mode, void *p_iop_addr, uint32 *p_spu_addr, uint32 size)
{
	Dbg_Assert(!s_sd_transferring_voice);

#if 0
	sceSdRemote(1, rSdVoiceTrans, channel, mode, p_iop_addr, p_spu_addr, size);
	sceSdRemote(1, rSdVoiceTransStatus, channel, SD_TRANS_STATUS_WAIT);
#else
	s_sd_transferring_voice = true;

	// Send transfer command first
	if (!s_queue_new_command(rSdVoiceTrans, channel, mode, (uint32) p_iop_addr, (uint32) p_spu_addr, size))
	{
		return false;
	}

	if (!s_queue_new_command(rSdVoiceTransStatus, channel, SD_TRANS_STATUS_WAIT))
	{
		return false;
	}

	s_wait_for_voice_transfer();
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CSpuManager::s_queue_new_command(uint16 function, uint16 entry, uint32 value, uint32 arg0, uint32 arg1, uint32 arg2)
{
	static int s_num_sound_requests_sent = 0;

	if (function == rSdSetSwitch)
	{
#if 0
		// If we are sending a switch, combine with old settings to make sure we don't overwrite them.
		// Switches are only updated every 1/48000 a second on the SPU.
		value = s_get_switch_value(entry, value);
#endif
		s_update_local_status(entry, value);
	}

	int nextFreeIndex = (s_sd_remote_command_free_index + 1) % MAX_QUEUED_COMMANDS;
	SSifCmdSoundRequestPacket *p_command = &s_sd_remote_commands[s_sd_remote_command_free_index];

	// Wait for old send to complete
	if (p_command->m_request.m_ee_dma_id)
	{
		//Dbg_Assert(0);
		int wait_num = 0;
		while(sceSifDmaStat(p_command->m_request.m_ee_dma_id) >= 0)
		{
			if ((wait_num++ % 1000) == 0)
				scePrintf("Waiting for sound request buffer\n");
		}
	}

	if (function == rSdSetEffectAttr)
	{
		sceSdEffectAttr *p_attr = (sceSdEffectAttr *) value;

		p_command->m_request.m_function	= function;
		p_command->m_request.m_entry		= entry;
		p_command->m_request.m_value		= p_attr->mode;
		p_command->m_request.m_arg_0		= ((uint32) p_attr->depth_L << 16) | (uint32) p_attr->depth_R;
		p_command->m_request.m_arg_1		= p_attr->delay;
		p_command->m_request.m_arg_2		= p_attr->feedback;
	}
	else
	{
		p_command->m_request.m_function	= function;
		p_command->m_request.m_entry		= entry;
		p_command->m_request.m_value		= value;
		p_command->m_request.m_arg_0		= arg0;
		p_command->m_request.m_arg_1		= arg1;
		p_command->m_request.m_arg_2		= arg2;
	}
	p_command->m_request.m_pad[0] = ++s_num_sound_requests_sent;

	// Send the actual command
	//scePrintf("CSpuManager: sending command %x with entry %x and value %x\n", function, entry, value);
	p_command->m_request.m_ee_dma_id = sceSifSendCmd(SOUND_REQUEST_COMMAND, p_command, sizeof(*p_command), 0, 0, 0);

	s_sd_remote_command_free_index = nextFreeIndex;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32			CSpuManager::s_get_switch_value(uint16 entry, uint32 value)
{
	int core = entry & 0x1;
	int switch_reg = entry & 0xFF00;

	uint32 new_value = value;

	switch (switch_reg)
	{
	case SD_S_KON:
		if(s_spu_reg_koff[core] & value)
		{
			//Dbg_MsgAssert(0, ("Turning on a voice that was turned off this frame."));
			Dbg_Message("Turning on a voice that was turned off this frame.");
			s_spu_reg_koff[core] &= ~(s_spu_reg_koff[core] & value);	// turn off those bits
		}

		s_spu_reg_kon[core] |= value;
		new_value = s_spu_reg_kon[core];

		break;

	case SD_S_KOFF:
		if(s_spu_reg_kon[core] & value)
		{
			//Dbg_MsgAssert(0, ("Turning off a voice that was turned on this frame."));
			Dbg_Message("Turning off a voice that was turned on this frame.");
			s_spu_reg_kon[core] &= ~(s_spu_reg_kon[core] & value);		// turn off those bits
		}

		s_spu_reg_koff[core] |= value;
		new_value = s_spu_reg_koff[core];

		break;

	default:
		break;
	}

	return new_value;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CSpuManager::s_update_local_status(uint16 entry, uint32 value)
{
	int core = entry & 0x1;
	int switch_reg = entry & 0xFF00;

	switch (switch_reg)
	{
	case SD_S_KON:
		//Dbg_Message("************ ORing KON%d with %x", core, value); 
		if(s_spu_reg_koff[core] & value)
		{
#ifdef	__NOPT_ASSERT__
			sPrintStatus();
#endif
			Dbg_Message("Core %d: Set KON value (%x)", core, value);
			Dbg_MsgAssert(0, ("Turning on a voice that was turned off this frame."));
			Dbg_Message("Turning on a voice that was turned off this frame.");
			s_spu_reg_koff[core] &= ~(s_spu_reg_koff[core] & value);	// turn off those bits
		}

		s_spu_reg_kon[core] |= value;

		// Make sure the voice is not used again before SPU update is done
		s_sd_voice_status[core] &= ~value;
		break;

	case SD_S_KOFF:
		//Dbg_Message("************ ORing KOFF%d with %x", core, value); 
		if(!(value & ~PS2Sfx_GetVoiceStatus(core)))
		{
#ifdef	__NOPT_ASSERT__
			sPrintStatus();
#endif
			Dbg_MsgAssert(0, ("Can't set KOFF with %x: status %x raw status %x", value, sGetVoiceStatus(core), s_sd_voice_status[core]));
			Dbg_Message("Can't set KOFF with %x: status %x raw status %x", value, sGetVoiceStatus(core), s_sd_voice_status[core]);
		}
		if(s_spu_reg_kon[core] & value)
		{
#ifdef	__NOPT_ASSERT__
			//sPrintStatus();
#endif
			//Dbg_Message("Core %d: Set KOFF value (%x)", core, value);
			//Dbg_MsgAssert(0, ("Turning off a voice that was turned on this frame."));
			Dbg_Message("Turning off a voice that was turned on this frame.");
			s_spu_reg_kon[core] &= ~(s_spu_reg_kon[core] & value);		// turn off those bits
		}

		s_spu_reg_koff[core] |= value;

		// Lets not clear the voice now.  Wait until we get the SPU status break;
		break;

	default:
		break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CSpuManager::s_result_callback(void *p, void *q)
{
	SSifCmdSoundResultPacket *p_result = (SSifCmdSoundResultPacket *) p;

	if (p_result->m_function == rSdVoiceTransStatus)
	{
		s_sd_transferring_voice = false;
	}
	else if (p_result->m_function == rSdGetSwitch)
	{
		int core = p_result->m_entry & 0x1;

		//scePrintf("************ CSpuManager: received result from command %x with entry %x and result %x for core %d\n", p_result->m_function, p_result->m_entry, p_result->m_result, core);

		if ((p_result->m_entry & 0xFF00) == SD_S_ENDX)
		{
			s_sd_voice_status[core] = p_result->m_result;
			s_sd_voice_status_update_frame[core] = Tmr::GetRenderFrame( );

			// We can clear this out now since the status is up-to-date
			VoicesUsed[ core ] &= ~VoicesCleared[ core ];
			//if (LoopingSounds[ core ] & VoicesCleared[ core ])
		//	{
		//		scePrintf("Clearing looping sound voices %x on core %d\n", LoopingSounds[ core ] & VoicesCleared[ core ], core);
		//	}
			LoopingSounds[ core ] &= ~VoicesCleared[ core ];
			VoicesCleared[ core ] = 0;

			// Keep bits that haven't been updated yet
			s_spu_reg_kon[core] &= (s_sd_voice_status[core] | ~VoicesUsed[core]);
			s_spu_reg_koff[core] &= ~(s_sd_voice_status[core] | ~VoicesUsed[core]);

			// And remove any leftover KON voices
			s_sd_voice_status[core] &= ~s_spu_reg_kon[core];
		}
	}

	if (p_result->m_panic)
	{
		Dbg_MsgAssert(0, ("Error on IOP module while processing SPU request %x", p_result->m_function));
	}

	ExitHandler();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CSpuManager::s_wait_for_voice_transfer()
{
	while (s_sd_transferring_voice)
		;
}

} // namespace Sfx
