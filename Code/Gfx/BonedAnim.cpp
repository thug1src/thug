//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       bonedanim.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/14/2001
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/bonedanim.h>

#include <gfx/bonedanimtypes.h>
#include <gfx/nxquickanim.h>
#include <sys/file/AsyncFilesys.h>
#include <sys/file/filesys.h>
#include <sys/mem/memman.h>
#include <gel/object.h>
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <sys/config/config.h>
#include <core/string/stringutils.h>
#include <sys/file/pip.h>

#ifdef __PLAT_NGC__
#include <dolphin.h>
#include "sys/ngc/p_dma.h"
#include "sys\ngc\p_aram.h"
#endif		// __PLAT_NGC__

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

#ifdef __PLAT_NGC__
#define __ARAM__
#endif

#ifdef __PLAT_NGC__
#define _16(a) (((a>>8)&0x00ff)|((a<<8)&0xff00))
#define _32(a) (((a>>24)&0x000000ff)|((a>>8)&0x0000ff00)|((a<<8)&0x00ff0000)|((a<<24)&0xff000000)) 
#else
#define _16(a) a
#define _32(a) a
#endif		// __PLAT_NGC__

namespace Gfx
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// In order to compress the data, we convert the animation quat into
// a unit quat and then discard the W value during export-time/load-time.  
// Given the X, Y, Z values, we can rebuild the W value at run-time.
// The code uses the following tolerance to when comparing the rebuilt W value
// to the original value.  I chose the number pretty arbitrarily, based
// on the existing (THPS3) skater and pedestrian animations.
//(0.0005 radians ~= 0.028 degrees)
const float nxUNITQUAT_TOLERANCE_RADIANS = 0.0005f;

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

// The following structures are only needed until new data
// is exported in the correct format

struct SBonedAnimFileHeader
{
	uint32 	    version;
	uint32    	flags;
	float	    duration;
};

struct SPlatformFileHeader
{
    uint32      numBones;
    uint32      numQKeys;
    uint32      numTKeys;
    uint32      numCustomAnimKeys;
};

struct SStandardAnimFramePointers
{
	unsigned char	numQKeys;
	unsigned char	numTKeys;
};

struct SHiResAnimFramePointers
{
	short			numQKeys;
	short			numTKeys;
};

//#define nxBONEDANIMFLAGS_UNUSED			(1<<31)
#define nxBONEDANIMFLAGS_INTERMEDIATE   	(1<<30)
#define nxBONEDANIMFLAGS_UNCOMPRESSED   	(1<<29)
#define nxBONEDANIMFLAGS_PLATFORM      		(1<<28)	
#define nxBONEDANIMFLAGS_CAMERADATA    		(1<<27)	
#define nxBONEDANIMFLAGS_COMPRESSEDTIME 	(1<<26)
#define nxBONEDANIMFLAGS_PREROTATEDROOT 	(1<<25)
#define nxBONEDANIMFLAGS_OBJECTANIMDATA		(1<<24)
#define nxBONEDANIMFLAGS_USECOMPRESSTABLE	(1<<23)
#define nxBONEDANIMFLAGS_HIRESFRAMEPOINTERS	(1<<22)
#define nxBONEDANIMFLAGS_CUSTOMKEYSAT60FPS	(1<<21)
#define nxBONEDANIMFLAGS_CUTSCENEDATA		(1<<20)
#define nxBONEDANIMFLAGS_PARTIALANIM		(1<<19)

// to be phased out...
#define nxBONEDANIMFLAGS_OLDPARTIALANIM		(1<<18)

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

#ifdef __ARAM__
#define ARAM_CACHE_SIZE (64*1024)
#define MAX_BONE_COUNT 128

static char qqq[ARAM_CACHE_SIZE] ATTRIBUTE_ALIGN(32); 

static uint16				framecount[MAX_BONE_COUNT*2] ATTRIBUTE_ALIGN(32);
//static uint32				partial[8] ATTRIBUTE_ALIGN(32);

volatile uint8				framecount_active = 0;
static int					framecount_size = 0;
ARQRequest					framecount_request;
#endif

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

// compress tables take up 4K,
// which are a waste on Xbox and GC
// (should PLAT_NGPS it once it's working)

struct CBonedAnimCompressEntry
{
	short x48;
	short y48;
	short z48;
	short n8;
};

CBonedAnimCompressEntry sQTable[256];
CBonedAnimCompressEntry sTTable[256];


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool init_48_table( void* pStream, CBonedAnimCompressEntry* pEntry )
{
	for ( int i = 0; i < 256; i++ )
	{
		if ( !File::Read( (int*)&pEntry->x48, sizeof(short), 1, pStream ) )
		{
			return false;
		}
		
		if ( !File::Read( (int*)&pEntry->y48, sizeof(short), 1, pStream ) )
		{
			return false;
		}
		
		if ( !File::Read( (int*)&pEntry->z48, sizeof(short), 1, pStream ) )
		{
			return false;
		}

		if ( !File::Read( (int*)&pEntry->n8, sizeof(short), 1, pStream ) )
		{
			return false;
		}
		
		pEntry->x48 = _16( pEntry->x48 );
		pEntry->y48 = _16( pEntry->y48 );
		pEntry->z48 = _16( pEntry->z48 );
		pEntry->n8 = _16( pEntry->n8 );

		pEntry++;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
#ifdef __ARAM__
//static volatile bool	dmaComplete;

static void arqCallback( u32 pointerToARQRequest )
{
//	ARQRequest * p_arq = (ARQRequest *)pointerToARQRequest;

//	if ( p_arq->owner == 0x55555555 )
//	{
		framecount_active = 0;
//	}
}

void dma_count( uint32 p_source_base, int size )
{
	// DMA the count info.
	size = ( ( size + 31 ) & ~31 ); 
	DCFlushRange ( framecount, size );
	framecount_size = size;
	framecount_active = 1;
	ARQPostRequest	(	&framecount_request,
						0x55555555,											// Owner.
						ARQ_TYPE_ARAM_TO_MRAM,								// Type.
						ARQ_PRIORITY_HIGH,									// Priority.
						p_source_base,										// Source.
						(uint32)framecount,									// Dest.
						size,												// Length.
						arqCallback );										// Callback
}

//void dma_partial( uint32 p_source_base )
//{
//	while ( framecount_active );
//	// DMA the count info.
//	DCFlushRange ( partial, 32 );
//	framecount_size = 32;
//	framecount_active = 1;
//	ARQPostRequest	(	&framecount_request,
//						0x55555555,											// Owner.
//						ARQ_TYPE_ARAM_TO_MRAM,								// Type.
//						ARQ_PRIORITY_HIGH,									// Priority.
//						p_source_base,										// Source.
//						(uint32)partial,									// Dest.
//						32,													// Length.
//						arqCallback );										// Callback
//}

#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool InitQ48Table( const char* pFileName, bool assertOnFail )
{
	bool success = false;
	
	// open the file as a stream
	void* pStream = File::Open( pFileName, "rb" );
	
    // make sure the file is valid
	if ( !pStream )
	{
		Dbg_MsgAssert( !assertOnFail, ("Load of %s failed - file not found?", pFileName) );
		goto exit;
	}

	success = init_48_table( pStream, &sQTable[0] );

exit:
	Dbg_MsgAssert( success, ("Parse of %s failed", pFileName) );
	File::Close( pStream );
	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool InitT48Table( const char* pFileName, bool assertOnFail )
{
	bool success = false;

	// open the file as a stream
	void* pStream = File::Open( pFileName, "rb" );

	// make sure the file is valid
	if ( !pStream )
	{
		Dbg_MsgAssert( assertOnFail, ("Load of %s failed - file not found?", pFileName) );
		goto exit;
	}

	success = init_48_table( pStream, &sTTable[0] );

	//Dbg_Assert( 0 );

exit:
	Dbg_MsgAssert( success, ("Parse of %s failed", pFileName) );
	File::Close( pStream );	
	return success;
}

/******************************************************************/
/*                                                                 */
/*                                                                */
/******************************************************************/

inline float timeDown(short theSource)
{
    return (float)theSource;
}	  

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline short timeUp(float theSource)
{
	short retVal = (short)((theSource * 60.0f) + 0.5f);

    return retVal;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline short transUp(float theSource, float scaleFactor)
{
    return (short)(theSource * scaleFactor);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float transDown(short theSource, float scaleFactor)
{
    return (float)(theSource / scaleFactor);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline short quatUp(float theSource)
{
    return (short)(theSource * 16384.0f);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float quatDown(short theSource)
{
    return (float)(theSource / 16384.0f);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void get_rotation_from_key( CAnimQKey* p_in, Mth::Quat* pQuat, bool isHiRes )
{
	if ( isHiRes )
	{
		(*pQuat)[X] = ((CHiResAnimQKey*)p_in)->qx;
		(*pQuat)[Y] = ((CHiResAnimQKey*)p_in)->qy;
		(*pQuat)[Z] = ((CHiResAnimQKey*)p_in)->qz;
//  	(*pQuat)[W] = ((CHiResAnimQKey*)p_in)->qw;
	}
	else
	{
		(*pQuat)[X] = quatDown( ((CStandardAnimQKey*)p_in)->qx );
		(*pQuat)[Y] = quatDown( ((CStandardAnimQKey*)p_in)->qy );
		(*pQuat)[Z] = quatDown( ((CStandardAnimQKey*)p_in)->qz );
//		(*pQuat)[W] = quatDown( ((CStandardAnimQKey*)p_in)->qw );
	}
	
	float qx = (*pQuat)[X];
	float qy = (*pQuat)[Y];
	float qz = (*pQuat)[Z];

	// Dave note: added 09/12/02 - a simple check to ensure we don't try to take the square root of a negative
	// number, which will hose Nan-sensitive platforms later on...
	float sum	= 1.0f - qx * qx - qy * qy - qz * qz;
	(*pQuat)[W] = sqrtf(( sum >= 0.0f ) ? sum : 0.0f );
	
	if ( p_in->signBit )
	{
		(*pQuat)[W] = -(*pQuat)[W];
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void set_key_from_rotation( CAnimQKey* p_in, float x, float y, float z, float w, bool isHiRes )
{
	if ( isHiRes )
	{
		((CHiResAnimQKey*)p_in)->qx = x;
		((CHiResAnimQKey*)p_in)->qy = y;
		((CHiResAnimQKey*)p_in)->qz = z;
//		((CHiResAnimQKey*)p_in)->qw = w;
		((CHiResAnimQKey*)p_in)->signBit = ( w < 0.0f ) ? 1 : 0;
	}
	else
	{
		((CStandardAnimQKey*)p_in)->qx = quatUp( x );
		((CStandardAnimQKey*)p_in)->qy = quatUp( y );
		((CStandardAnimQKey*)p_in)->qz = quatUp( z );
//		((CStandardAnimQKey*)p_in)->qw = quatUp( w );
		((CStandardAnimQKey*)p_in)->signBit = ( w < 0.0f ) ? 1 : 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void get_translation_from_key( CAnimTKey* p_in, Mth::Vector* pVector, bool isHiRes )
{
	if ( isHiRes )
	{
		(*pVector)[X] = ((CHiResAnimTKey*)p_in)->tx;
		(*pVector)[Y] = ((CHiResAnimTKey*)p_in)->ty;
		(*pVector)[Z] = ((CHiResAnimTKey*)p_in)->tz;
		(*pVector)[W] = 1.0f;
	}
	else
	{
		(*pVector)[X] = transDown( ((CStandardAnimTKey*)p_in)->tx, 32.0f );
		(*pVector)[Y] = transDown( ((CStandardAnimTKey*)p_in)->ty, 32.0f );
		(*pVector)[Z] = transDown( ((CStandardAnimTKey*)p_in)->tz, 32.0f );
		(*pVector)[W] = 1.0f;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline void get_rotation_from_standard_key( CStandardAnimQKey* p_in, Mth::Quat* pQuat )
{
	if ( p_in->qx == 0 && p_in->qy == 0 && p_in->qz == 0 )
	{
		// Optimization: this handles the identity rotation, which is so common that it's not
		// worth doing a square root...
		pQuat->SetVector( 0.0f, 0.0f, 0.0f );
		pQuat->SetScalar( 1.0f );
		return;
	}
		
	float qx = quatDown( p_in->qx );
	float qy = quatDown( p_in->qy );
	float qz = quatDown( p_in->qz );

	// Dave note: added 09/12/02 - a simple check to ensure we don't try
	// to take the square root of a negative number, which will hose 
	// Nan-sensitive platforms later on...
	float sum = 1.0f - qx * qx - qy * qy - qz * qz;
	
// This assert was firing off in one of the cutscenes...
// probably worth looking into later...
//	Dbg_Assert( sum >= 0.0f );
	if ( sum < 0.0f )
	{
		sum = 0.0f;
	}

	(*pQuat)[X] = qx;
	(*pQuat)[Y] = qy;
	(*pQuat)[Z] = qz;
	(*pQuat)[W] = ( p_in->signBit ) ? -sqrtf( sum ) : sqrtf( sum );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void get_translation_from_standard_key( CStandardAnimTKey* p_in, Mth::Vector* pVector )
{
	(*pVector)[X] = transDown( p_in->tx, 32.0f );
	(*pVector)[Y] = transDown( p_in->ty, 32.0f );
	(*pVector)[Z] = transDown( p_in->tz, 32.0f );
	(*pVector)[W] = 1.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
						  
inline void set_key_from_translation( CAnimTKey* p_in, float x, float y, float z, bool isHiRes )
{
	if ( isHiRes )
	{
		((CHiResAnimTKey*)p_in)->tx = x;
		((CHiResAnimTKey*)p_in)->ty = y;
		((CHiResAnimTKey*)p_in)->tz = z;
	}
	else
	{
		((CStandardAnimTKey*)p_in)->tx = transUp( x, 32.0f );
		((CStandardAnimTKey*)p_in)->ty = transUp( y, 32.0f );
		((CStandardAnimTKey*)p_in)->tz = transUp( z, 32.0f );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline void interpolate_q_frame(Mth::Quat* p_out, CAnimQKey* p_in1, CAnimQKey* p_in2, float alpha, bool isHiRes)
{
	if ( alpha == 0.0f )
	{
		// don't need to slerp, because it's the start time
		get_rotation_from_key( p_in1, p_out, isHiRes );
		return;
	}

	if ( alpha == 1.0f )
	{
		// don't need to slerp, because it's the end time
		get_rotation_from_key( p_in2, p_out, isHiRes );
		return;
	}
	
	// INTERPOLATE Q-COMPONENT

	Mth::Quat	qIn1;
	Mth::Quat   qIn2;
	
	get_rotation_from_key( p_in1, &qIn1, isHiRes );
	get_rotation_from_key( p_in2, &qIn2, isHiRes );
    
	// Faster slerp, stolen from game developer magazine.
	*p_out = Mth::FastSlerp( qIn1, qIn2, alpha );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void interpolate_t_frame(Mth::Vector* p_out, CAnimTKey* p_in1, CAnimTKey* p_in2, float alpha, bool isHiRes)
{
	if ( alpha == 0.0f )
	{
		// don't need to lerp, because it's the start time
		get_translation_from_key( p_in1, p_out, isHiRes );
		return;
	}

	if ( alpha == 1.0f )
	{
		// don't need to slerp, because it's the end time
		get_translation_from_key( p_in2, p_out, isHiRes );
		return;
	}

	// INTERPOLATE T-COMPONENT

    Mth::Vector   tIn1;					  
	Mth::Vector   tIn2;
	
	get_translation_from_key( p_in1, &tIn1, isHiRes );
	
	get_translation_from_key( p_in2, &tIn2, isHiRes );

    /* Linearly interpolate positions */
    *p_out = Mth::Lerp( tIn1, tIn2, alpha );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
						  
inline void interpolate_standard_q_frame(Mth::Quat* p_out, CStandardAnimQKey* p_in1, CStandardAnimQKey* p_in2, float alpha)
{
	if ( alpha == 0.0f )
	{
		// don't need to slerp, because it's the start time
		get_rotation_from_standard_key( p_in1, p_out );
		return;
	}

	if ( alpha == 1.0f )
	{
		// don't need to slerp, because it's the end time
		get_rotation_from_standard_key( p_in2, p_out );
		return;
	}
	
	// INTERPOLATE Q-COMPONENT
	Mth::Quat	qIn1;
	Mth::Quat   qIn2;
	get_rotation_from_standard_key( p_in1, &qIn1 );
	get_rotation_from_standard_key( p_in2, &qIn2 );

	// Faster slerp, stolen from game developer magazine.
	*p_out = Mth::FastSlerp( qIn1, qIn2, alpha );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void interpolate_standard_t_frame(Mth::Vector* p_out, CStandardAnimTKey* p_in1, CStandardAnimTKey* p_in2, float alpha)
{
	if ( alpha == 0.0f )
	{
		// don't need to lerp, because it's the start time
		get_translation_from_standard_key( p_in1, p_out );
		return;
	}

	if ( alpha == 1.0f )
	{
		// don't need to slerp, because it's the end time
		get_translation_from_standard_key( p_in2, p_out );
		return;
	}

	// INTERPOLATE T-COMPONENT

    Mth::Vector   tIn1;					  
	Mth::Vector   tIn2;
	
	get_translation_from_standard_key( p_in1, &tIn1 );
	
	get_translation_from_standard_key( p_in2, &tIn2 );

    /* Linearly interpolate positions */
    *p_out = Mth::Lerp( tIn1, tIn2, alpha );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// eventually, this will go in the plat-specific version of CBonedAnimFrameData

bool CBonedAnimFrameData::plat_dma_to_aram( int qbytes, int tbytes, uint32 flags )
{
	// GameCube: DMA to ARAM.
#ifdef __ARAM__

	if ( qbytes && tbytes )
	{
		uint32 address;
		uint size;
//   	uint32 header[8];

//		// Want this to happen even if asserts turned off.
//		if ( qbytes > (ARAM_CACHE_SIZE) )
//		{
//			OSReport( "Too many q keys (%d)!!!\n", qbytes );
//			while (1);
//		}
//
//		if ( tbytes > (ARAM_CACHE_SIZE) )
//		{
//			OSReport( "Too many t keys (%d)!!!\n", tbytes );
//			while (1);
//		}
//
//		if ( m_numBones > MAX_BONE_COUNT )
//		{
//			OSReport( "Too many bones (%d)!!!\n", m_numBones );
//			while (1);
//		}

//		// DMA partial animation data.
//		if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
//		{
//			address = NsARAM::alloc( 32 );
//			NsDMA::toARAM( address, mp_partialAnimData, 32 );
//			mp_partialAnimData = (uint32*)address;
//		}

		// DMA per-bone frame count data.
		size = m_numBones * sizeof(uint16) * 2;
		memcpy( qqq, mp_perBoneQFrameSize, size );
		int part_size = 0;
		int frame_size = size;
		if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
		{
			part_size = ( 1 + (( *mp_partialAnimData - 1 )/ 32) + 1 ) * sizeof( uint32 );
			memcpy( &qqq[size], mp_partialAnimData, part_size );
			size += part_size;
		}
		size = ( ( size + 31 ) & ~31 ); 
		address = NsARAM::alloc( size );
		NsDMA::toARAM( address, qqq, size );

		mp_perBoneQFrameSize = (uint16*)address;
		mp_perBoneTFrameSize = (uint16*)address;
		if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
		{
			mp_partialAnimData = (uint32*)( address + frame_size );
		}

//		size = m_numBones * sizeof(uint16) * 2;
//		size = ( ( size + 31 ) & ~31 ); 
//		address = NsARAM::alloc( size );
//		NsDMA::toARAM( address, mp_perBoneQFrameSize, size );
//
//		mp_perBoneQFrameSize = (uint16*)address;
//		mp_perBoneTFrameSize = (uint16*)address;

		// DMA Q frames to ARAM, delete RAM version & assign ARAM pointer.
		size = qbytes/* + 32*/;
		size = ( ( size + 31 ) & ~31 ); 
//		header[0] = size;
		address = NsARAM::alloc( size );
//		NsDMA::toARAM( address, header, 32 );
		NsDMA::toARAM( address/*+32*/, mp_qFrames, size );
//		delete mp_qFrames;
		mp_qFrames = (char*)address;

		// DMA T frames to ARAM, delete RAM version & assign ARAM pointer.
		size = tbytes/* + 32*/;
		size = ( ( size + 31 ) & ~31 ); 
//		header[0] = size;
		address = NsARAM::alloc( size );
//		NsDMA::toARAM( address, header, 32 );
		NsDMA::toARAM( address/*+32*/, mp_tFrames, size );
//		delete mp_tFrames;
		mp_tFrames = (char*)address;

		// hijack this pointer as it won't be used.
		mp_boneNames = (uint32*)( ( qbytes << 16 ) | tbytes );
	}
	else
	{
		if ( is_hires() )
		{
			uint32 address;
			uint size;

//			// Want this to happen even if asserts turned off.
//			if ( ( m_num_qFrames * sizeof( CHiResAnimQKey ) ) > (ARAM_CACHE_SIZE) )
//			{
//				OSReport( "Too many q keys (%d)!!!\n", m_num_qFrames );
//				while (1);
//			}
//
//			if ( ( m_num_tFrames * sizeof( CHiResAnimTKey ) ) > (ARAM_CACHE_SIZE) )
//			{
//				OSReport( "Too many t keys (%d)!!!\n", m_num_tFrames );
//				while (1);
//			}
//

//			// DMA partial animation data.
//			if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
//			{
//				uint32 * pData = mp_partialAnimData;
//				int numBones = *pData;
//
//				int numMasks = (( numBones - 1 )/ 32) + 1;		
//				size = sizeof( int ) + ( numMasks * sizeof( uint32 ) ); 
//
//				header[0] = size;
//				address = NsARAM::alloc( size );
//				NsDMA::toARAM( address, header, 32 );
//				NsDMA::toARAM( address+32, mp_partialAnimData, size );
//				mp_partialAnimData = (char*)address;
//			}
			mp_partialAnimData = NULL;

			// DMA per-bone frame count data.
			void * ptr;
			if ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA )
			{
				size = ( m_numBones * sizeof(uint32) );
				ptr = mp_boneNames;
			}
			else
			{
				size = 0;
				ptr = mp_perBoneFrames;
			}

			if ( m_flags & nxBONEDANIMFLAGS_HIRESFRAMEPOINTERS )
			{
				size += ( m_numBones * sizeof(SHiResAnimFramePointers) );
			}
			else
			{
				size += ( m_numBones * sizeof(SStandardAnimFramePointers) );
			}

			if ( size > ( MAX_BONE_COUNT * 2 * sizeof(uint16) ) )
			{
				OSReport( "Too many bones (%d)!!!\n", m_numBones );
				while (1);
			}

			size = ( ( size + 31 ) & ~31 ); 
			address = NsARAM::alloc( size );
			NsDMA::toARAM( address, ptr, size );

			mp_perBoneFrames = (uint16*)address;
			mp_perBoneQFrameSize = (uint16*)address;		// So that it can be deallocated.

			// DMA Q frames to ARAM, delete RAM version & assign ARAM pointer.
//			Dbg_MsgAssert( m_num_qFrames <= (ARAM_CACHE_SIZE), ( "Too many hires Q keys (%d) - maximum is %d", m_num_qFrames, (ARAM_CACHE_SIZE) ) );
			size = sizeof( CHiResAnimQKey ) * m_num_qFrames;
			size = ( ( size + 31 ) & ~31 ); 
			address = NsARAM::alloc( size );
			NsDMA::toARAM( address, mp_qFrames, size );
//			delete mp_qFrames;
			mp_qFrames = (char*)address;

			// DMA T frames to ARAM, delete RAM version & assign ARAM pointer.
//			Dbg_MsgAssert( m_num_tFrames <= (ARAM_CACHE_SIZE), ( "Too many hires T keys (%d) - maximum is %d", m_num_tFrames, (ARAM_CACHE_SIZE) ) );
			size = sizeof( CHiResAnimTKey ) * m_num_tFrames;
			size = ( ( size + 31 ) & ~31 ); 
			address = NsARAM::alloc( size );
			NsDMA::toARAM( address, mp_tFrames, size );
//			delete mp_tFrames;
			mp_tFrames = (char*)address;
		}
		else
		{
			uint32 address;
			uint size;

//			// Want this to happen even if asserts turned off.
//			if ( ( m_num_qFrames * sizeof( CStandardAnimQKey ) ) > (ARAM_CACHE_SIZE) )
//			{
//				OSReport( "Too many q keys (%d)!!!\n", m_num_qFrames );
//				while (1);
//			}
//
//			if ( ( m_num_tFrames * sizeof( CStandardAnimTKey ) ) > (ARAM_CACHE_SIZE) )
//			{
//				OSReport( "Too many t keys (%d)!!!\n", m_num_tFrames );
//				while (1);
//			}
//

//			// DMA partial animation data.
//			if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
//			{
//				uint32 * pData = mp_partialAnimData;
//				int numBones = *pData;
//
//				int numMasks = (( numBones - 1 )/ 32) + 1;		
//				size = sizeof( int ) + ( numMasks * sizeof( uint32 ) ); 
//
//				header[0] = size;
//				address = NsARAM::alloc( size );
//				NsDMA::toARAM( address, header, 32 );
//				NsDMA::toARAM( address+32, mp_partialAnimData, size );
//				mp_partialAnimData = (char*)address;
//			}
			mp_partialAnimData = NULL;

			// DMA per-bone frame count data.
			void * ptr;
			if ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA )
			{
				size = ( m_numBones * sizeof(uint32) );
				ptr = mp_boneNames;
			}
			else
			{
				size = 0;
				ptr = mp_perBoneFrames;
			}

			if ( m_flags & nxBONEDANIMFLAGS_HIRESFRAMEPOINTERS )
			{
				size += ( m_numBones * sizeof(SHiResAnimFramePointers) );
			}
			else
			{
				size += ( m_numBones * sizeof(SStandardAnimFramePointers) );
			}

			if ( size > ( MAX_BONE_COUNT * 2 * sizeof(uint16) ) )
			{
				OSReport( "Too many bones (%d)!!!\n", m_numBones );
				while (1);
			}

			size = ( ( size + 31 ) & ~31 ); 
			address = NsARAM::alloc( size );
			NsDMA::toARAM( address, ptr, size );

			mp_perBoneFrames = (uint16*)address;
			mp_perBoneQFrameSize = (uint16*)address;		// So that it can be deallocated.

			// DMA Q frames to ARAM, delete RAM version & assign ARAM pointer.
//			Dbg_MsgAssert( m_num_qFrames <= (short)MAX_STANDARD_Q, ( "Too many standard Q keys (%d) - maximum is %d", m_num_qFrames, MAX_STANDARD_Q ) );
			size = sizeof( CStandardAnimQKey ) * m_num_qFrames;
			size = ( ( size + 31 ) & ~31 ); 
			address = NsARAM::alloc( size );
			NsDMA::toARAM( address, mp_qFrames, size );
//			delete mp_qFrames;
			mp_qFrames = (char*)address;

			// DMA T frames to ARAM, delete RAM version & assign ARAM pointer.
//			Dbg_MsgAssert( m_num_tFrames <= (short)MAX_STANDARD_T, ( "Too many standard T keys (%d) - maximum is %d", m_num_tFrames, MAX_STANDARD_T ) );
			size = sizeof( CStandardAnimTKey ) * m_num_tFrames;
			size = ( ( size + 31 ) & ~31 ); 
			address = NsARAM::alloc( size );
			NsDMA::toARAM( address, mp_tFrames, size );
//			delete mp_tFrames;
			mp_tFrames = (char*)address;
		}
	}

#endif		// __ARAM__
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CBonedAnimFrameData::plat_read_compressed_stream(uint8* pData, bool delete_buffer)
{
	Dbg_Assert( pData );

	SPlatformFileHeader *pThePlatformHeader = (SPlatformFileHeader *) pData;
	pData += sizeof(SPlatformFileHeader);

	m_numBones = pThePlatformHeader->numBones;
	m_num_qFrames = pThePlatformHeader->numQKeys;
	m_num_tFrames = pThePlatformHeader->numTKeys;
	
	uint32 qAllocSize = *((uint32*)pData);
	pData += sizeof(uint32);

	uint32 tAllocSize = *((uint32*)pData);
	pData += sizeof(uint32);

	mp_perBoneQFrameSize = (uint16*)pData;
	pData += ( m_numBones * sizeof(uint16) );
	
	mp_perBoneTFrameSize = (uint16*)pData;
	pData += ( m_numBones * sizeof(uint16) );

	// long-align
	if ( (uint32)pData & 0x3 )
	{
		pData += ( 4 - ((uint32)pData & 0x3) );
	}
	
	// first read in object anim bone names, if any
	if ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA )
	{
		Dbg_MsgAssert( 0, ( "Wasn't expecting object anims to be compressed" ) );
		mp_boneNames = NULL;
	}
	else
	{
		mp_boneNames = NULL;
	}
	
	if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
	{
		Dbg_Assert(!((uint32)pData & 0x3));
		
		mp_partialAnimData = (uint32*)pData;
		
		// skip original number of bones
		int numBones = *((uint32*)pData);
		pData += sizeof( int );
		
		int numMasks = (( numBones - 1 )/ 32) + 1;		
		pData += ( numMasks * sizeof( uint32 ) );
	}

	// long-align
	if ( (uint32)pData & 0x3 )
	{
		pData += ( 4 - ((uint32)pData & 0x3) );
	}
		
	if ( is_hires() )
	{
		Dbg_MsgAssert( !is_hires(), ( "Hi res format not supported for compressed anims" ) );
	}
	else
	{
		mp_qFrames = (char*)pData;
		pData += qAllocSize;
		
		mp_tFrames = (char*)pData;
		pData += tAllocSize;
	}
	
	Dbg_Assert( mp_perBoneFrames == NULL );
	Dbg_Assert( mp_qFrames );
	Dbg_Assert( mp_tFrames );
    
	// dma to aram here...
	plat_dma_to_aram( qAllocSize, tAllocSize );

	// GJ:  be able to PIP the custom keys in as well

	// long-align
	if ( (uint32)pData & 0x3 )
	{
		pData += ( 4 - ((uint32)pData & 0x3) );
	}
   
	// create an array of pointers to the custom keys
	if ( !pThePlatformHeader->numCustomAnimKeys )
	{
		mpp_customAnimKeyList = NULL;
	}
	else
	{
		mpp_customAnimKeyList = (CCustomAnimKey **)(Mem::Malloc(pThePlatformHeader->numCustomAnimKeys*sizeof(CCustomAnimKey *)));
		
		// read custom keys
		for ( uint32 i = 0; i < pThePlatformHeader->numCustomAnimKeys; i++ )
		{
			CCustomAnimKey* pKey = ReadCustomAnimKey( &pData );
			if ( !pKey )
			{
				Dbg_Message( "Failed while reading custom data" );
				return false;
			}		
			mpp_customAnimKeyList[i] = pKey;
		}
	}		

	m_num_customKeys = pThePlatformHeader->numCustomAnimKeys;

#ifdef __ARAM__
	if ( delete_buffer )
	{
		delete mp_fileBuffer;
	}
	mp_fileBuffer = NULL;
#endif
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBonedAnimFrameData::plat_read_stream(uint8* pData, bool delete_buffer)
{
	Dbg_Assert( pData );

	SPlatformFileHeader* pThePlatformHeader = (SPlatformFileHeader*)pData;
	pData += sizeof(SPlatformFileHeader);
	Dbg_Assert(!((uint) pData & 0x3));

	m_numBones = pThePlatformHeader->numBones;
	m_num_qFrames = pThePlatformHeader->numQKeys;
	m_num_tFrames = pThePlatformHeader->numTKeys;
	
	Dbg_Assert( !mp_perBoneFrames );
	Dbg_Assert( !mp_qFrames );
	Dbg_Assert( !mp_tFrames );
	
	// first read in object anim bone names, if any
	if ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA )
	{
		Dbg_Assert(!((uint) pData & 0x3));
		mp_boneNames = (uint32*)pData;
		pData += ( m_numBones * sizeof(uint32) );
	}
	else
	{
		mp_boneNames = NULL;
	}

	if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
	{
		Dbg_Assert(!((uint32)pData & 0x3));
		
		mp_partialAnimData = (uint32*)pData;
		
		// skip original number of bones
		int numBones = *((uint32*)pData);
		pData += sizeof( int );
		
		int numMasks = (( numBones - 1 )/ 32) + 1;		
		pData += ( numMasks * sizeof( uint32 ) );
	}

	// now, read in the per-bone frames of the correct size
	if ( m_flags & nxBONEDANIMFLAGS_HIRESFRAMEPOINTERS )
	{
		mp_perBoneFrames = pData;
		pData += ( m_numBones * sizeof(SHiResAnimFramePointers) );		
	}
	else
	{
		mp_perBoneFrames = pData;
		pData += ( m_numBones * sizeof(SStandardAnimFramePointers) );		
	}

	// long-align
	if ( (uint32)pData & 0x3 )
	{
		pData += ( 4 - ((uint32)pData & 0x3) );
	}
   
	// count to make sure the number of keys per bone didn't overflow
	int runningQCount = 0;
	int runningTCount = 0;

	for ( int i = 0; i < m_numBones; i++ )
	{
		runningQCount += get_num_qkeys( mp_perBoneFrames, i );
		runningTCount += get_num_tkeys( mp_perBoneFrames, i );
	}

	Dbg_MsgAssert( runningQCount == m_num_qFrames, ( "Wrong number of qframes in %x %d %d", m_fileNameCRC, runningQCount, m_num_qFrames ) );
	Dbg_MsgAssert( runningTCount == m_num_tFrames, ( "Wrong number of tframes in %x %d %d", m_fileNameCRC, runningTCount, m_num_tFrames ) );

	if ( is_hires() )
	{
		Dbg_Assert(!((uint) pData & 0x3));
		mp_qFrames = (char*)pData;
		pData += ( m_num_qFrames * sizeof(CHiResAnimQKey) );
		
		Dbg_Assert(!((uint) pData & 0x3));
		mp_tFrames = (char*)pData;
		pData += ( m_num_tFrames * sizeof(CHiResAnimTKey) );
	}
	else
	{
		Dbg_Assert(!((uint) pData & 0x3));
		mp_qFrames = (char*)pData;
		pData += ( m_num_qFrames * sizeof(CStandardAnimQKey) );
		
		Dbg_Assert(!((uint) pData & 0x3));
		mp_tFrames = (char*)pData;
		pData += ( m_num_tFrames * sizeof(CStandardAnimTKey) );
	}
	
	Dbg_Assert( mp_perBoneFrames );
	Dbg_Assert( mp_qFrames );
	Dbg_Assert( mp_tFrames );
    
	// dma to aram here...
	plat_dma_to_aram( 0, 0, m_flags );

	// GJ:  be able to PIP the custom keys in as well

	// long-align
	if ( (uint32)pData & 0x3 )
	{
		pData += ( 4 - ((uint32)pData & 0x3) );
	}
   
	// create an array of pointers to the custom keys
	if ( !pThePlatformHeader->numCustomAnimKeys )
	{
		mpp_customAnimKeyList = NULL;
	}
	else
	{
		mpp_customAnimKeyList = (CCustomAnimKey **)(Mem::Malloc(pThePlatformHeader->numCustomAnimKeys*sizeof(CCustomAnimKey *)));
		
		// read custom keys
		for ( uint32 i = 0; i < pThePlatformHeader->numCustomAnimKeys; i++ )
		{
			CCustomAnimKey* pKey = ReadCustomAnimKey( &pData );
			if ( !pKey )
			{
				Dbg_Message( "Failed while reading custom data" );
				return false;
			}		
			mpp_customAnimKeyList[i] =  pKey;
		}
	}		

	m_num_customKeys = pThePlatformHeader->numCustomAnimKeys;

#ifdef __ARAM__
	if ( delete_buffer )
	{
		delete mp_fileBuffer;
	}
	mp_fileBuffer = NULL;
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CBonedAnimFrameData::get_num_qkeys( void * p_frame_data, int boneIndex ) const
{
	if ( m_flags & nxBONEDANIMFLAGS_HIRESFRAMEPOINTERS )
	{
		SHiResAnimFramePointers* pFramePointers = (SHiResAnimFramePointers*)p_frame_data;
		
		pFramePointers += boneIndex;

		return pFramePointers->numQKeys;
	}
	else
	{
		SStandardAnimFramePointers* pFramePointers = (SStandardAnimFramePointers*)p_frame_data;
		
		pFramePointers += boneIndex;

		return pFramePointers->numQKeys;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CBonedAnimFrameData::get_num_tkeys( void * p_frame_data, int boneIndex ) const
{
	if ( m_flags & nxBONEDANIMFLAGS_HIRESFRAMEPOINTERS )
	{
		SHiResAnimFramePointers* pFramePointers = (SHiResAnimFramePointers*)p_frame_data;
		
		pFramePointers += boneIndex;

		return pFramePointers->numTKeys;
	}
	else
	{
		SStandardAnimFramePointers* pFramePointers = (SStandardAnimFramePointers*)p_frame_data;
		
		pFramePointers += boneIndex;

		return pFramePointers->numTKeys;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CBonedAnimFrameData::is_hires() const
{
	return ( m_flags & nxBONEDANIMFLAGS_CAMERADATA ) || ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA );
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBonedAnimFrameData::CBonedAnimFrameData()
{
	m_duration = 0.0f;
	m_numBones = 0;

	mp_fileBuffer = NULL;
	mp_fileHandle = NULL;
	m_dataLoaded = false;

	mp_qFrames = NULL;	
	mp_tFrames = NULL;

	mp_perBoneFrames = NULL;
	mp_boneNames = NULL;

	mp_perBoneQFrameSize = NULL;
	mp_perBoneTFrameSize = NULL;

	m_printDebugInfo = false;

	m_num_customKeys = 0;

	mpp_customAnimKeyList = NULL;
	m_fileNameCRC = 0;
	m_pipped = false;

	mp_partialAnimData = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBonedAnimFrameData::~CBonedAnimFrameData()
{

	for (int i=0;i<m_num_customKeys;i++)
	{
		delete mpp_customAnimKeyList[i];
	}

	if ( mpp_customAnimKeyList )
	{
		Mem::Free( mpp_customAnimKeyList );
	}

	if ( m_pipped )
	{
		Pip::Unload( m_fileNameCRC );
	}
	else
	{
		if (mp_fileBuffer)
		{
			Mem::Free(mp_fileBuffer);
		}
	}

	if (mp_fileHandle)
	{
		Dbg_MsgAssert(m_dataLoaded, ("Can't delete CBonedAnimFrameData while it is still being loaded"));
		File::CAsyncFileLoader::sClose( mp_fileHandle );
	}

#ifdef __ARAM__
	// This relies on all level-specific anims being deallocated (a reasonable dependency).
	// Animations can be deleted in any order, the lowest address is the base address for the next level.
	NsARAM::free( (uint32)mp_perBoneQFrameSize );
	NsARAM::free( (uint32)mp_qFrames );
	NsARAM::free( (uint32)mp_tFrames );
//	if ( ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM ) && mp_partialAnimData )
//	{
//		NsARAM::free( (uint32)mp_partialAnimData ); 
//	}
#else

#endif		// __ARAM__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CBonedAnimFrameData::Load(uint32* pData, int file_size, bool assertOnFail)
{
	// TODO:  We should read in the entire file into memory first, because
	// using streams is slow.

	Dbg_Assert( !m_dataLoaded );

	m_fileNameCRC = 0;

	Dbg_MsgAssert( pData, ( "No data pointer specified" ) );
	Dbg_MsgAssert(file_size, ("Anim file size is 0"));
	Dbg_Assert(!mp_fileBuffer);

#ifdef __ARAM__
	mp_fileBuffer = pData;
	return PostLoad(assertOnFail, file_size, false);
#else
//	Now that we're pipping it, we don't have to force this on the top-down heap
//	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	mp_fileBuffer = (void *) Mem::Malloc( file_size );
//	Mem::Manager::sHandle().PopContext();
	
	// copy the data into the temporary buffer
	memcpy( mp_fileBuffer, pData, file_size );

//	printf( "Loading data:  %08x %08x %08x %d\n", *pData, mp_fileBuffer, pData, file_size );

	return PostLoad(assertOnFail, file_size);
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CBonedAnimFrameData::Load(const char* p_fileName, bool assertOnFail, bool async, bool use_pip)
{         
	// TODO:  We should read in the entire file into memory first, because
	// using streams is slow.

	Dbg_Assert( p_fileName );
	Dbg_Assert( !m_dataLoaded );

	char test[256];
	strcpy( test, p_fileName );
	Str::LowerCase( test );
	m_fileNameCRC = Crc::GenerateCRCFromString( test );

	int file_size = 0;

	char filename[256];
	strcpy( filename, p_fileName );

	// Turn off async for platforms that don't support it
	if (!File::CAsyncFileLoader::sAsyncSupported())
	{
		async = false;
	}

	if ( async )
	{
		Dbg_MsgAssert( !use_pip, ( "Pip-files do not work with async-loaded anims" ) );

		// Pre-load
		Dbg_Assert(!mp_fileHandle);

		mp_fileHandle = File::CAsyncFileLoader::sOpen( p_fileName, !async );

		// make sure the file is valid
		if ( !mp_fileHandle )
		{
			Dbg_MsgAssert( assertOnFail, ("Load of %s failed - file not found?", p_fileName) );
			return false;
		}

		file_size = mp_fileHandle->GetFileSize();

		Dbg_MsgAssert(file_size, ("Anim file size is 0"));

		Dbg_Assert(!mp_fileBuffer);
//		Now that we're pipping it, we don't have to force this on the top-down heap
//		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
		mp_fileBuffer = (void *) Mem::Malloc( file_size );
//		Mem::Manager::sHandle().PopContext();

		// Set the callback
		mp_fileHandle->SetCallback(async_callback, (unsigned int) this, (unsigned int) assertOnFail);

		// read the file in
		mp_fileHandle->Read( mp_fileBuffer, 1, file_size );
		//Dbg_Message("Started read of %x", this);

		//mp_fileHandle->WaitForIO();
		//Dbg_Message("Done waiting for %x", this);

		// Should be the callback
		return true; //PostLoad(assertOnFail, file_size);
	}
	else
	{
		int file_size = 0;

		if ( use_pip )
		{
			mp_fileBuffer = Pip::Load( p_fileName );
			file_size = Pip::GetFileSize( p_fileName );
			Dbg_MsgAssert(file_size, ("Anim file size is 0"));
			m_pipped = true;
		}
		else
		{
			// open the file as a stream
			void* pStream = File::Open( p_fileName,"rb" );
			// make sure the file is valid
			if ( !pStream )
			{
				Dbg_MsgAssert( assertOnFail, ("Load of %s failed - file not found?", p_fileName) );
				return false;
			}

			file_size = File::GetFileSize(pStream);
			Dbg_MsgAssert(file_size, ("Anim file size is 0"));
			Dbg_Assert(!mp_fileBuffer);
	//		Now that we're pipping it, we don't have to force this on the top-down heap
	//		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
			mp_fileBuffer = (void *) Mem::Malloc( file_size );
	//		Mem::Manager::sHandle().PopContext();

			// read the file in
			if ( !File::Read( mp_fileBuffer, 1, file_size, pStream ) )
			{
				Mem::Free(mp_fileBuffer);
				mp_fileBuffer = NULL;
				File::Close( pStream );

				Dbg_MsgAssert( assertOnFail, ("Load of %s failed - bad format?", p_fileName) );
				return false;
			}
			
			File::Close(pStream);
		}

		return PostLoad(assertOnFail, file_size);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBonedAnimFrameData::async_callback(File::CAsyncFileHandle *, File::EAsyncFunctionType function,
										 int result, unsigned int arg0, unsigned int arg1)
{
	//Dbg_Message("Got callback from %x", arg0);
	if (function == File::FUNC_READ)
	{
		CBonedAnimFrameData *p_data = (CBonedAnimFrameData *) arg0;
		bool assert = (bool) arg1;

		p_data->PostLoad(assert, result);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBonedAnimFrameData::PostLoad(bool assertOnFail, int file_size, bool delete_buffer)
{
	bool success = false;

	//Dbg_Message("PostLoad of %x", this);

	// Handle end of async, if that was used
	if (mp_fileHandle)
	{
#if 0
		if (mp_fileHandle->WaitForIO() != file_size)
		{
			Mem::Free(mp_fileBuffer);
			mp_fileBuffer = NULL;
			File::CAsyncFileLoader::sClose( mp_fileHandle );
			mp_fileHandle = NULL;

			Dbg_MsgAssert( assertOnFail, ("PostLoad of anim file failed - bad format?") );
			return false;
		}
#endif
		File::CAsyncFileLoader::sClose( mp_fileHandle );
		mp_fileHandle = NULL;
	}

	uint8 *pFileData = (uint8 *) mp_fileBuffer;

	// read in header information
	SBonedAnimFileHeader* pTheHeader = (SBonedAnimFileHeader*)pFileData;
	pFileData += sizeof(SBonedAnimFileHeader);

	// store header information
	m_duration = pTheHeader->duration;

	m_flags = pTheHeader->flags;

	// trying to phase out the old partial anim format here...
	Dbg_MsgAssert( ( m_flags & nxBONEDANIMFLAGS_OLDPARTIALANIM ) == 0, ( "No longer supporting old partial anim format" ) ); 

	// the anim converter should automatically do this,
	// so that we don't have to do it at runtime...
	Dbg_MsgAssert( pTheHeader->flags & nxBONEDANIMFLAGS_COMPRESSEDTIME, ( "Expected the anim times to be in frames" ) );
	Dbg_MsgAssert( pTheHeader->flags & nxBONEDANIMFLAGS_PREROTATEDROOT, ( "Expected the root bones to be prerotated" ) );

	// some debugging information
//	Dbg_Message( "Loading animation %s", p_fileName );
//	Dbg_Message( "Duration = %f", m_duration );
//	Dbg_Message( "Flags = %08x", theHeader.flags );

	// read the stream differently based on whether it's compressed...
    if ( pTheHeader->flags & nxBONEDANIMFLAGS_PLATFORM )
    {
		Dbg_Assert( (pTheHeader->flags & nxBONEDANIMFLAGS_USECOMPRESSTABLE) == 0 );
		success = this->plat_read_stream( pFileData, delete_buffer );
    }
    else if ( pTheHeader->flags & nxBONEDANIMFLAGS_USECOMPRESSTABLE )
    {
		Dbg_Assert( (pTheHeader->flags & nxBONEDANIMFLAGS_PLATFORM) == 0 );
		success = this->plat_read_compressed_stream( pFileData, delete_buffer );
    }
    else
    {
        Dbg_MsgAssert( 0, ("Unrecognized file format (flags = %08x)...", pTheHeader->flags) );
    }

	// handle failure
	Dbg_MsgAssert( success || !assertOnFail, ("PostLoad of anim file failed - bad format?") );

	if ( !m_pipped )
	{
	// Done with temp buffer (since it's pipped, don't delete the file buffer)
//	Mem::Free(mp_fileBuffer);
//	mp_fileBuffer = NULL;
	}
	m_dataLoaded = success;

    return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBonedAnimFrameData::IsValidTime( float time )
{
	return ( time >= 0.0f && time < m_duration );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float get_alpha( float timeStamp1, float timeStamp2, float time )
{
	Dbg_MsgAssert(timeStamp1 <= time && timeStamp2 >= time, ( "%f should be within [%f %f]", time, timeStamp1, timeStamp2 ));
    
	return (( time - timeStamp1 ) / ( timeStamp2 - timeStamp1 ));
}

// THE FOLLOWING 2 FUNCTIONS SHOULD BE REFACTORED?

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBonedAnimFrameData::GetInterpolatedCameraFrames(Mth::Quat* pRotations, Mth::Vector* pTranslations, float time, Nx::CQuickAnim* pQuickAnim )
{
    Dbg_Assert( pRotations );
    Dbg_Assert( pTranslations );

	Dbg_Assert( is_hires() );
	
	float qAlpha = 0.0f;
	float tAlpha = 0.0f;

	// DMA the animation data here.
#ifdef __ARAM__
	int		size;
	if ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA )
	{
		size = ( m_numBones * sizeof(uint32) );
	}
	else
	{
		size = 0;
	}

	if ( m_flags & nxBONEDANIMFLAGS_HIRESFRAMEPOINTERS )
	{
		size += ( m_numBones * sizeof(SHiResAnimFramePointers) );
	}
	else
	{
		size += ( m_numBones * sizeof(SStandardAnimFramePointers) );
	}
	dma_count( (uint32)mp_perBoneFrames, size );
	while ( framecount_active );
//	if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
//	{
//		dma_partial( (uint32)mp_partialAnimData, ( m_numBones * sizeof( uint16 ) * 2 ) );
//	}
//	while ( framecount_active );

//	NsDMA::toMRAM( qqq, (uint32)mp_qFrames, m_num_qFrames * sizeof( CHiResAnimQKey ) );
//	NsDMA::toMRAM( ttt, (uint32)mp_tFrames, m_num_tFrames * sizeof( CHiResAnimTKey ) );

//	while( framecount_active );
#endif		// __ARAM__

	// find the nearest 2 keyframes
	float qTimeStamp = time * 60.0f;
	float tTimeStamp = time * 60.0f;
	
	CHiResAnimQKey* pStartQFrame = NULL;
	CHiResAnimQKey* pEndQFrame = NULL;

	CHiResAnimTKey* pStartTFrame = NULL;
	CHiResAnimTKey* pEndTFrame = NULL;

	CHiResAnimQKey* pCurrentQFrame = NULL;
	CHiResAnimTKey* pCurrentTFrame = NULL;
	
#ifdef __ARAM__
//	pStartQFrame = p_hqqq;
//	pEndQFrame = p_hqqq;
//
//	pStartTFrame = p_httt;
//	pEndTFrame = p_httt;
//	
//	pCurrentQFrame = p_hqqq;
//	pCurrentTFrame = p_httt;

	uint32 anim_q_offset = (uint32)mp_qFrames;
	uint32 anim_t_offset = (uint32)mp_tFrames;

	void * p_bone_frames;
	if ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA )
	{
		p_bone_frames = &framecount[m_numBones*2];
	}
	else
	{
		p_bone_frames = framecount;
	}

#else
	pStartQFrame = (CHiResAnimQKey*)mp_qFrames;
	pEndQFrame = pStartQFrame;
	pCurrentQFrame = pStartQFrame;

	pStartTFrame = (CHiResAnimTKey*)mp_tFrames;
	pEndTFrame = pStartTFrame;
	pCurrentTFrame = pStartTFrame;

	void * p_bone_frames = mp_perBoneFrames;
#endif		// __aram__

	for ( int i = 0; i < m_numBones; i++ )
	{
		int numQKeys = get_num_qkeys( p_bone_frames, i );
		int numTKeys = get_num_tkeys( p_bone_frames, i );

#ifdef __ARAM__
		int q_off = 0;
		{
			// DMA this q track.
			int aligned_off = ( anim_q_offset & ~31 );
			int size = ( ( ( sizeof( CHiResAnimQKey ) * numQKeys ) - ( aligned_off - anim_q_offset ) ) + 31 ) & ~31;
			DCFlushRange ( qqq, size );
			framecount_size = size;
			framecount_active = 1;
			ARQPostRequest	(	&framecount_request,
								0x55555555,											// Owner.
								ARQ_TYPE_ARAM_TO_MRAM,								// Type.
								ARQ_PRIORITY_HIGH,									// Priority.
								(u32)aligned_off,									// Source.
								(uint32)qqq,										// Dest.
								size,												// Length.
								arqCallback );										// Callback
			while ( framecount_active );
			q_off = anim_q_offset - aligned_off;

			pStartQFrame = (CHiResAnimQKey*)&qqq[q_off];
			pEndQFrame = (CHiResAnimQKey*)&qqq[q_off];
			pCurrentQFrame = (CHiResAnimQKey*)&qqq[q_off];
		}
#endif		// __ARAM__

		for ( int j = 0; j < numQKeys; j++ )
		{
			if ( j == (numQKeys-1) )
			{
				// last frame
				pStartQFrame = pCurrentQFrame + j;
				pEndQFrame = pCurrentQFrame + j;
				qAlpha = 0.0f;
				break;
			}
			else if ( qTimeStamp >= timeDown(pCurrentQFrame[j].timestamp) 
				 && qTimeStamp <= timeDown(pCurrentQFrame[j+1].timestamp) )
			{
				pStartQFrame = pCurrentQFrame + j;
				pEndQFrame = pCurrentQFrame + j + 1;
				qAlpha = get_alpha( timeDown(pStartQFrame->timestamp), timeDown(pEndQFrame->timestamp), qTimeStamp );
				break;
			}
		}

		interpolate_q_frame( pRotations, 
						 pStartQFrame, 
						 pEndQFrame,
						 qAlpha,
						 is_hires() );

#ifdef __ARAM__
		int t_off = 0;
		{
			// DMA this t track.
			int aligned_off = ( anim_t_offset & ~31 );
			int size = ( ( ( sizeof( CHiResAnimTKey ) * numTKeys ) - ( aligned_off - anim_t_offset ) ) + 31 ) & ~31;
			DCFlushRange ( qqq, size );
			framecount_size = size;
			framecount_active = 1;
			ARQPostRequest	(	&framecount_request,
								0x55555555,											// Owner.
								ARQ_TYPE_ARAM_TO_MRAM,								// Type.
								ARQ_PRIORITY_HIGH,									// Priority.
								(u32)aligned_off,									// Source.
								(uint32)qqq,										// Dest.
								size,												// Length.
								arqCallback );										// Callback
			while ( framecount_active );
			t_off = anim_t_offset - aligned_off;

			pStartTFrame = (CHiResAnimTKey*)&qqq[t_off];
			pEndTFrame = (CHiResAnimTKey*)&qqq[t_off];
			pCurrentTFrame = (CHiResAnimTKey*)&qqq[t_off];
		}
#endif		// __ARAM__

		for ( int j = 0; j < numTKeys; j++ )
		{
			if ( j == (numTKeys-1) )
			{
				// last frame
				pStartTFrame = pCurrentTFrame + j;
				pEndTFrame = pCurrentTFrame + j;
				tAlpha = 0.0f;
				break;
			}
			else if ( tTimeStamp >= timeDown(pCurrentTFrame[j].timestamp) 
				 && tTimeStamp <= timeDown(pCurrentTFrame[j+1].timestamp) )
			{
				pStartTFrame = pCurrentTFrame + j;
				pEndTFrame = pCurrentTFrame + j + 1;
				tAlpha = get_alpha( timeDown(pStartTFrame->timestamp), timeDown(pEndTFrame->timestamp), tTimeStamp );
				break;
			}
		}
		
		// theStartFrame and theEndFrame should contain the
		// two closest keyframes here.  now interpolate between them
		// TODO:  we might be able to cache some of this data...
		interpolate_t_frame( pTranslations, 
							 pStartTFrame, 
							 pEndTFrame,
							 tAlpha,
							 is_hires() );

#if 0
		if ( m_printDebugInfo )
		{
			// for debug info, if necessary
		}
#endif
		
#ifdef __ARAM__
		anim_q_offset += ( sizeof( CHiResAnimQKey ) * numQKeys );
		anim_t_offset += ( sizeof( CHiResAnimTKey ) * numTKeys );
#else
		pCurrentQFrame += numQKeys;
		pCurrentTFrame += numTKeys;
#endif	// __ARAM__
		pRotations++;
		pTranslations++;
	}

    return true;
}

#ifdef __ARAM__
inline int get_compressed_q_frame( char* pData, CStandardAnimQKey* pReturnData )
#else
inline char * get_compressed_q_frame( char* pData, CStandardAnimQKey* pReturnData )
#endif		// __ARAM__
{
	unsigned char* p_data = (unsigned char*)pData;

	short data = (short)(( p_data[1]<<8 ) | p_data[0] );
	pReturnData->timestamp = data & 0x3fff;
	pReturnData->signBit = data & 0x8000 ? 1 : 0;
	
	// skip the timestamp
	p_data += sizeof( short );

	if ( data & 0x4000 )
	{
		if ( !(data & 0x3800) )
		{
			unsigned char d = *p_data;

			// a char* defaults to a signed char*,
			// which means you'll try
			// to access outside the array
			// boundaries...  brutal!
			//Dbg_Assert( *p_data == d );

			CBonedAnimCompressEntry* pEntry = &sQTable[d];
			pReturnData->qx = pEntry->x48;
			pReturnData->qy = pEntry->y48;
			pReturnData->qz = pEntry->z48;
			p_data += sizeof( char );

			// remove the bits
			pReturnData->timestamp &= 0x07ff;
		}
		else
		{
			if ( data & 0x2000 )
			{
				pReturnData->qx = p_data[0];
				p_data += sizeof( char );
			}
			else
			{
				pReturnData->qx = (p_data[1]<<8) | p_data[0];
				p_data += sizeof( short );
			}

			if ( data & 0x1000 )
			{
				pReturnData->qy = p_data[0];
				p_data += sizeof( char );
			}
			else
			{
				pReturnData->qy = (p_data[1]<<8) | p_data[0];
				p_data += sizeof( short );
			}

			if ( data & 0x0800 )
			{
				pReturnData->qz = p_data[0];
				p_data += sizeof( char );
			}
			else
			{
				pReturnData->qz = (p_data[1]<<8) | p_data[0];
				p_data += sizeof( short );
			}

			// remove the bits
			pReturnData->timestamp &= 0x07ff;
		}
	}
	else
	{
		// no compression
		pReturnData->qx = (p_data[1]<<8) | p_data[0];
		pReturnData->qy = (p_data[3]<<8) | p_data[2];
		pReturnData->qz = (p_data[5]<<8) | p_data[4];
		p_data += 6;
	}

#ifdef __ARAM__
	return (int)p_data - (int)pData;
#else
	return (char*)p_data;
#endif		// __ARAM__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __ARAM__
inline int get_compressed_t_frame( char* pData, CStandardAnimTKey* pReturnData )
#else
inline char * get_compressed_t_frame( char* pData, CStandardAnimTKey* pReturnData )
#endif		// __ARAM__
{
	unsigned char* p_data = (unsigned char*)pData;
	
	bool useLookupTable = false;
	
	unsigned char flags = *p_data;
	p_data += sizeof( char );
	
	if ( flags & 0x80 )
	{					
		useLookupTable = true;
	}

	if ( flags & 0x40 )
	{
		// expand the timestamp
		pReturnData->timestamp = (unsigned short)(flags & 0x3f);
	}
	else
	{
		// read another short
		pReturnData->timestamp = (unsigned short)(p_data[1]<<8) | p_data[0];
		p_data += sizeof( short );
	}

	if ( useLookupTable )
	{
		unsigned char d = *p_data;

		// defaults to signed char,
		// which means you'll try
		// to access outside the array
		// boundaries...  brutal!
		//Dbg_Assert( *p_data == d );
		
		CBonedAnimCompressEntry* pEntry = &sTTable[d];
		pReturnData->tx = pEntry->x48;
		pReturnData->ty = pEntry->y48;
		pReturnData->tz = pEntry->z48;
		p_data += sizeof( char );
	}
	else
	{
		// no compression
		pReturnData->tx = (p_data[1]<<8) | p_data[0];
		pReturnData->ty = (p_data[3]<<8) | p_data[2];
		pReturnData->tz = (p_data[5]<<8) | p_data[4];
		p_data += 6;
	}

#ifdef __ARAM__
	return (int)p_data - (int)pData;
#else
	return (char*)p_data;
#endif		// __ARAM__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBonedAnimFrameData::GetCompressedInterpolatedFrames(Mth::Quat* pRotations, Mth::Vector* pTranslations, float time, Nx::CQuickAnim* pQuickAnim )
{
    Dbg_Assert( pRotations );
    Dbg_Assert( pTranslations );

	Dbg_Assert( !is_hires() );

	CStandardAnimQKey theStartQFrame;
	CStandardAnimQKey theEndQFrame;
	CStandardAnimQKey* pStartQFrame = &theStartQFrame;
	CStandardAnimQKey* pEndQFrame = &theEndQFrame;
	
	CStandardAnimTKey theStartTFrame;
	CStandardAnimTKey theEndTFrame;
	CStandardAnimTKey* pStartTFrame = &theStartTFrame;
	CStandardAnimTKey* pEndTFrame = &theEndTFrame;
	
#ifdef __ARAM__
	char* pCurrentQFrame = mp_qFrames;//&mp_qFrames[32];
	char* pCurrentTFrame = mp_tFrames;//&mp_tFrames[32];

	// Pre-dma framecount sizes.
	dma_count( (uint32)mp_perBoneQFrameSize, ( m_numBones * sizeof( uint16 ) * 2 ) + 16 );		// +16 should cover partial mask data.
//	if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
//	{
//		while ( framecount_active );
//		dma_partial( (uint32)mp_partialAnimData );
//	}
//	uint32 * partial = (uint32 *)&framecount[( (uint32)mp_partialAnimData - (uint32)mp_perBoneQFrameSize ) * 2];
	uint32 * partial = (uint32 *)&framecount[( (uint32)mp_partialAnimData - (uint32)mp_perBoneQFrameSize ) / 2];

	// Pre-DMA all data.
//	uint32 q_lines = ((uint32)mp_boneNames) >> 16;
//	uint32 t_lines = ((uint32)mp_boneNames) & 0xffff;
//	q_lines = ( ( q_lines + ( ARAM_CACHE_LINE_SIZE - 1 ) ) / ARAM_CACHE_LINE_SIZE ); 
//	t_lines = ( ( t_lines + ( ARAM_CACHE_LINE_SIZE - 1 ) ) / ARAM_CACHE_LINE_SIZE ); 

//	for ( uint32 lp = 0; lp < q_lines; lp++ ) dma_q_entry( ((uint32)mp_qFrames), lp );
//	for ( uint32 lp = 0; lp < t_lines; lp++ ) dma_t_entry( ((uint32)mp_tFrames), lp );

	uint16* pQSizes = &framecount[0];
	uint16* pTSizes = &framecount[m_numBones];

	// Make sure framecount sizes have been DMA'd.
	while ( framecount_active );
#else
	Dbg_Assert( mp_qFrames );
	Dbg_Assert( mp_tFrames );

	char* pCurrentQFrame = (char*)mp_qFrames;
	char* pCurrentTFrame = (char*)mp_tFrames;

	uint16* pQSizes = &mp_perBoneQFrameSize[0];
	uint16* pTSizes = &mp_perBoneTFrameSize[0];
#endif		// __aram__
	
	// find the nearest 2 keyframes
	float qTimeStamp = time * 60.0f;
	float tTimeStamp = time * 60.0f;
	
	// Precalculate the skip index mask for speed.
	uint32 skip_index_mask = ( pQuickAnim && pQuickAnim->m_quickAnimPointers.valid ) ? ( 1 << pQuickAnim->m_quickAnimPointers.skipIndex ) : 0;

	for ( int i = 0; i < m_numBones; i++ )
	{
		// See if the QuickAnim data indicates that this bone may be skipped.
		bool skip_this_bone = ( skip_index_mask ) ? (( pQuickAnim->m_quickAnimPointers.pSkipList[i] & skip_index_mask ) > 0 ) : false;

		if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
		{
#ifdef __ARAM__
			uint32* pBoneMask = ( partial + 1 ) + ( i / 32 );
#else
			uint32* pBoneMask = ( mp_partialAnimData + 1 ) + ( i / 32 );
#endif		// __ARAM__
			skip_this_bone = ( ( (*pBoneMask) & ( 1 << (i%32) ) ) == 0 );
//			skip_this_bone = ( ( (*pBoneMask) & ( (1<<31) >> (i%32) ) ) == 0 );
		}

		if( !skip_this_bone )
		{
			float qAlpha = 0.0f;
			float tAlpha = 0.0f;

			char* pNextQFrame = pCurrentQFrame;
			char* pNextQBone = pCurrentQFrame + *pQSizes;
		
			char* pNextTFrame = pCurrentTFrame;
    		char* pNextTBone = pCurrentTFrame + *pTSizes;

			if ( pQuickAnim && pQuickAnim->m_quickAnimPointers.valid )
			{
				pNextQFrame = pQuickAnim->m_quickAnimPointers.pQuickQKey[i];
				pNextTFrame = pQuickAnim->m_quickAnimPointers.pQuickTKey[i];
			}
	 
#ifdef __ARAM__
			int q_off = 0;
			{
				// DMA this q track.
				uint aligned_off = ( (int)pNextQFrame & ~31 );
				uint size = ( ( *pQSizes - ( aligned_off - (int)pCurrentQFrame ) ) + 31 ) & ~31;
				DCFlushRange ( qqq, size );
				framecount_size = size;
				framecount_active = 1;
				ARQPostRequest	(	&framecount_request,
									0x55555555,											// Owner.
									ARQ_TYPE_ARAM_TO_MRAM,								// Type.
									ARQ_PRIORITY_HIGH,									// Priority.
									(u32)aligned_off,									// Source.
									(uint32)qqq,										// Dest.
									size,												// Length.
									arqCallback );										// Callback
				q_off = (int)pNextQFrame - aligned_off;
			}
#endif		// __ARAM__

			while ( 1 )
			{
				if ( pQuickAnim )
				{
					pQuickAnim->m_quickAnimPointers.pQuickQKey[i] = (char*)pNextQFrame;
				}

#				ifdef __ARAM__
				while ( framecount_active );
#				endif		// __ARAM__

#ifdef __ARAM__
				int bytes = get_compressed_q_frame( &qqq[q_off], pStartQFrame );
				q_off += bytes;
				pNextQFrame = &pNextQFrame[bytes];
#else
				pNextQFrame = get_compressed_q_frame( pNextQFrame, pStartQFrame );
#endif		// __ARAM__
				if ( pNextQFrame >= pNextQBone )
				{
					// last frame
					*pEndQFrame = *pStartQFrame;
					qAlpha = 0.0f;
					break;
				}

#ifdef __ARAM__
				get_compressed_q_frame( &qqq[q_off], pEndQFrame );
#else
				get_compressed_q_frame( pNextQFrame, pEndQFrame );
#endif		// __ARAM__
				if ( qTimeStamp >= timeDown(pStartQFrame->timestamp) && qTimeStamp <= timeDown(pEndQFrame->timestamp) )
				{
					qAlpha = get_alpha( timeDown(pStartQFrame->timestamp), timeDown(pEndQFrame->timestamp), qTimeStamp );
					break;
				}
			}

			// theStartFrame and theEndFrame should contain the
			// two closest keyframes here.  now interpolate between them
			// TODO:  we might be able to cache some of this data...
			interpolate_standard_q_frame( pRotations, pStartQFrame, pEndQFrame, qAlpha );
			
#ifdef __ARAM__
			int t_off = 0;
			{
				// DMA this q track.
				int aligned_off = ( (int)pNextTFrame & ~31 );
				int size = ( ( *pTSizes - ( aligned_off - (int)pCurrentTFrame ) ) + 31 ) & ~31;
				DCFlushRange ( qqq, size );
				framecount_size = size;
				framecount_active = 1;
				ARQPostRequest	(	&framecount_request,
									0x55555555,											// Owner.
									ARQ_TYPE_ARAM_TO_MRAM,								// Type.
									ARQ_PRIORITY_HIGH,									// Priority.
									(u32)aligned_off,									// Source.
									(uint32)qqq,										// Dest.
									size,												// Length.
									arqCallback );										// Callback
				t_off = (int)pNextTFrame - aligned_off;
			}
#endif		// __ARAM__

			while ( 1 )
			{
				if ( pQuickAnim )
				{
					pQuickAnim->m_quickAnimPointers.pQuickTKey[i] = (char*)pNextTFrame;
				}

#				ifdef __ARAM__
				while ( framecount_active );
#				endif		// __ARAM__

#ifdef __ARAM__
				int bytes = get_compressed_t_frame( &qqq[t_off], pStartTFrame );
				t_off += bytes;
				pNextTFrame = &pNextTFrame[bytes];
#else
				pNextTFrame = get_compressed_t_frame( pNextTFrame, pStartTFrame );
#endif		// __ARAM__
				if ( pNextTFrame >= pNextTBone )
				{
					// last frame
					*pEndTFrame = *pStartTFrame;
					tAlpha = 0.0f;
					break;
				}

#ifdef __ARAM__
				get_compressed_t_frame( &qqq[t_off], pEndTFrame );
#else
				get_compressed_t_frame( pNextTFrame, pEndTFrame );
#endif		// __ARAM__
				if ( tTimeStamp >= timeDown(pStartTFrame->timestamp) && tTimeStamp <= timeDown(pEndTFrame->timestamp) )
				{
					tAlpha = get_alpha( timeDown(pStartTFrame->timestamp), timeDown(pEndTFrame->timestamp), tTimeStamp );
					break;
				}
			}

			interpolate_standard_t_frame( pTranslations, pStartTFrame, pEndTFrame, tAlpha );
		}
		
		pCurrentQFrame += *pQSizes;
		pCurrentTFrame += *pTSizes;
		pQSizes++;
		pTSizes++;
		pRotations++;
		pTranslations++;
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBonedAnimFrameData::GetInterpolatedFrames(Mth::Quat* pRotations, Mth::Vector* pTranslations, float time, Nx::CQuickAnim* pQuickAnim )
{
	if ( m_flags & nxBONEDANIMFLAGS_USECOMPRESSTABLE )
	{
		// new partial anims now use the standard GetCompressedInterpolatedFrames function
		return GetCompressedInterpolatedFrames(pRotations, pTranslations, time, pQuickAnim);
	}

	if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
	{
		Dbg_MsgAssert( 0, ( "Non-compressed partial animations are not supported yet" ) );
	}

    Dbg_Assert( pRotations );
    Dbg_Assert( pTranslations );

	Dbg_Assert( !is_hires() );
    
	float qAlpha = 0.0f;
	float tAlpha = 0.0f;

	// DMA the animation data here.
#ifdef __ARAM__
	int		size;
	if ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA )
	{
		size = ( m_numBones * sizeof(uint32) );
	}
	else
	{
		size = 0;
	}

	if ( m_flags & nxBONEDANIMFLAGS_HIRESFRAMEPOINTERS )
	{
		size += ( m_numBones * sizeof(SHiResAnimFramePointers) );
	}
	else
	{
		size += ( m_numBones * sizeof(SStandardAnimFramePointers) );
	}
	dma_count( (uint32)mp_perBoneFrames, size );
	while ( framecount_active );
//	if ( m_flags & nxBONEDANIMFLAGS_PARTIALANIM )
//	{
//		dma_partial( (uint32)mp_partialAnimData, ( m_numBones * sizeof( uint16 ) * 2 ) );
//	}
//	while ( framecount_active );

//	NsDMA::toMRAM( p_sqqq, (uint32)mp_qFrames, m_num_qFrames * sizeof( CStandardAnimQKey ) );
//	NsDMA::toMRAM( p_sttt, (uint32)mp_tFrames, m_num_tFrames * sizeof( CStandardAnimTKey ) );

//	while( framecount_active );
#endif		// __ARAM__

	// find the nearest 2 keyframes
	float qTimeStamp = time * 60.0f;
	float tTimeStamp = time * 60.0f;
	
	CStandardAnimQKey* pStartQFrame = NULL;
	CStandardAnimQKey* pEndQFrame = NULL;

	CStandardAnimTKey* pStartTFrame = NULL;
	CStandardAnimTKey* pEndTFrame = NULL;

	CStandardAnimQKey* pCurrentQFrame = NULL;
	CStandardAnimTKey* pCurrentTFrame = NULL;
	
#ifdef __ARAM__
//	pStartQFrame = p_sqqq;
//	pEndQFrame = p_sqqq;
//
//	pStartTFrame = p_sttt;
//	pEndTFrame = p_sttt;
//	
//	pCurrentQFrame = p_sqqq;
//	pCurrentTFrame = p_sttt;

	uint32 anim_q_offset = (uint32)mp_qFrames;
	uint32 anim_t_offset = (uint32)mp_tFrames;

	void * p_bone_frames;
	if ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA )
	{
		p_bone_frames = &framecount[m_numBones*2];
	}
	else
	{
		p_bone_frames = framecount;
	}
#else
	pStartQFrame = (CStandardAnimQKey*)mp_qFrames;
	pEndQFrame = (CStandardAnimQKey*)mp_qFrames;

	pStartTFrame = (CStandardAnimTKey*)mp_tFrames;
	pEndTFrame = (CStandardAnimTKey*)mp_tFrames;
	
	pCurrentQFrame = (CStandardAnimQKey*)mp_qFrames;
	pCurrentTFrame = (CStandardAnimTKey*)mp_tFrames;

	void * p_bone_frames = mp_perBoneFrames;
#endif		// __aram__

	for ( int i = 0; i < m_numBones; i++ )
	{
		int numQKeys = get_num_qkeys( p_bone_frames, i );
		int numTKeys = get_num_tkeys( p_bone_frames, i );

#ifdef __ARAM__
		int q_off = 0;
		{
			// DMA this q track.
			int aligned_off = ( anim_q_offset & ~31 );
			int size = ( ( ( sizeof( CStandardAnimQKey ) * numQKeys ) - ( aligned_off - anim_q_offset ) ) + 31 ) & ~31;
			DCFlushRange ( qqq, size );
			framecount_size = size;
			framecount_active = 1;
			ARQPostRequest	(	&framecount_request,
								0x55555555,											// Owner.
								ARQ_TYPE_ARAM_TO_MRAM,								// Type.
								ARQ_PRIORITY_HIGH,									// Priority.
								(u32)aligned_off,									// Source.
								(uint32)qqq,										// Dest.
								size,												// Length.
								arqCallback );										// Callback
			while ( framecount_active );
			q_off = anim_q_offset - aligned_off;

			pStartQFrame = (CStandardAnimQKey*)&qqq[q_off];
			pEndQFrame = (CStandardAnimQKey*)&qqq[q_off];
			pCurrentQFrame = (CStandardAnimQKey*)&qqq[q_off];
		}
#endif		// __ARAM__

		for ( int j = 0; j < numQKeys; j++ )
		{
			if ( j == (numQKeys-1) )
			{
				// last frame
				pStartQFrame = pCurrentQFrame + j;
				pEndQFrame = pCurrentQFrame + j;
				qAlpha = 0.0f;
				break;
			}
			else if ( qTimeStamp >= timeDown(pCurrentQFrame[j].timestamp) 
				 && qTimeStamp <= timeDown(pCurrentQFrame[j+1].timestamp) )
			{
				pStartQFrame = pCurrentQFrame + j;
				pEndQFrame = pCurrentQFrame + j + 1;
				qAlpha = get_alpha( timeDown(pStartQFrame->timestamp), timeDown(pEndQFrame->timestamp), qTimeStamp );
				break;
			}
		}

		interpolate_q_frame( pRotations, 
						 pStartQFrame, 
						 pEndQFrame,
						 qAlpha,
						 is_hires() );

#ifdef __ARAM__
		int t_off = 0;
		{
			// DMA this t track.
			int aligned_off = ( anim_t_offset & ~31 );
			int size = ( ( ( sizeof( CStandardAnimTKey ) * numTKeys ) - ( aligned_off - anim_t_offset ) ) + 31 ) & ~31;
			DCFlushRange ( qqq, size );
			framecount_size = size;
			framecount_active = 1;
			ARQPostRequest	(	&framecount_request,
								0x55555555,											// Owner.
								ARQ_TYPE_ARAM_TO_MRAM,								// Type.
								ARQ_PRIORITY_HIGH,									// Priority.
								(u32)aligned_off,									// Source.
								(uint32)qqq,										// Dest.
								size,												// Length.
								arqCallback );										// Callback
			while ( framecount_active );
			t_off = anim_t_offset - aligned_off;

			pStartTFrame = (CStandardAnimTKey*)&qqq[t_off];
			pEndTFrame = (CStandardAnimTKey*)&qqq[t_off];
			pCurrentTFrame = (CStandardAnimTKey*)&qqq[t_off];
		}
#endif		// __ARAM__

		for ( int j = 0; j < numTKeys; j++ )
		{
			if ( j == (numTKeys-1) )
			{
				// last frame
				pStartTFrame = pCurrentTFrame + j;
				pEndTFrame = pCurrentTFrame + j;
				tAlpha = 0.0f;
				break;
			}
			else if ( tTimeStamp >= timeDown(pCurrentTFrame[j].timestamp) 
				 && tTimeStamp <= timeDown(pCurrentTFrame[j+1].timestamp) )
			{
				pStartTFrame = pCurrentTFrame + j;
				pEndTFrame = pCurrentTFrame + j + 1;
				tAlpha = get_alpha( timeDown(pStartTFrame->timestamp), timeDown(pEndTFrame->timestamp), tTimeStamp );
				break;
			}
		}
		
		// theStartFrame and theEndFrame should contain the
		// two closest keyframes here.  now interpolate between them
		// TODO:  we might be able to cache some of this data...
		interpolate_t_frame( pTranslations, 
							 pStartTFrame, 
							 pEndTFrame,
							 tAlpha,
							 is_hires() );

#if 0
		if ( m_printDebugInfo )
		{
			// for debug info, if necessary
		}
#endif

#ifdef __ARAM__
		anim_q_offset += ( sizeof( CStandardAnimQKey ) * numQKeys );
		anim_t_offset += ( sizeof( CStandardAnimTKey ) * numTKeys );
#else
		pCurrentQFrame += numQKeys;
		pCurrentTFrame += numTKeys;
#endif	// __ARAM__
		pRotations++;
		pTranslations++;
	}

    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CBonedAnimFrameData::GetBoneName( int index )
{
	// only the object anim stores the name of the bones
	Dbg_MsgAssert( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA, ( "Bone names are only stored with object anims" ) );
	
	Dbg_MsgAssert( index >= 0 && index < m_numBones, ( "Bone name request out of range %d (0-%d)", index, m_numBones ) );

	Dbg_Assert( mp_boneNames );

#ifdef __ARAM__
	int		size;
	if ( m_flags & nxBONEDANIMFLAGS_OBJECTANIMDATA )
	{
		size = ( m_numBones * sizeof(uint32) );
	}
	else
	{
		size = 0;
	}
	dma_count( (uint32)mp_perBoneFrames, size );
	while ( framecount_active );

	uint32 * p32 = (uint32*)framecount;
	return p32[index];
#else
	return mp_boneNames[index];
#endif		// __ARAM__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CBonedAnimFrameData::get_num_customkeys()
{
	return m_num_customKeys;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCustomAnimKey* CBonedAnimFrameData::get_custom_key( int index )
{
	Dbg_Assert( index >= 0 && index < get_num_customkeys() );
	Dbg_MsgAssert( mpp_customAnimKeyList, ( "custom key list doesn't exist" ) );
	return mpp_customAnimKeyList[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBonedAnimFrameData::ResetCustomKeys()
{
	// this assumes that there's only going to be one 
	// animcontroller calling this frame data...
	// eventually, we might need to move this
	// into CReferencedFrameData...

	int customKeyCount = get_num_customkeys();
	for ( int i = 0; i < customKeyCount; i++ )
	{					 
		CCustomAnimKey* pKey = get_custom_key(i);
		pKey->SetActive( true );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBonedAnimFrameData::ProcessCustomKeys( float startTime, float endTime, Obj::CObject* pObject, bool inclusive )
{
	// for each key,
	// see if it's between the start and end time
	// start_time <= time < end_time

	// get it into frames...
	startTime *= 60.0f;
	endTime *= 60.0f;

	int customKeyCount = get_num_customkeys();
	for ( int i = 0; i < customKeyCount; i++ )
	{
		CCustomAnimKey* pKey = get_custom_key(i);
		if ( pKey->WithinRange( startTime, endTime, inclusive ) )
		{
//			printf( "Processing key at %f (%f %f)\n", custom_key_time, startTime, endTime );
			pKey->ProcessKey( pObject );
		}
		else
		{
//			printf( "Not processing key at %f (%f %f)\n", custom_key_time, startTime, endTime );
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx





