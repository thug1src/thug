/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SK3														**
**																			**
**	Module:			Game Engine (GEL)	 									**
**																			**
**	File name:		p_movies.cpp											**
**																			**
**	Created:		08/27/01	-	dc										**
**																			**
**	Description:	Gamecube specific movie streaming code					**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef DVDETH

#undef __GX_H__
#define _GX_H_

#include <dolphin.h>
#include <stdio.h>
#include <string.h>
#include <core/defines.h>
#include <sys/ngc/p_display.h>
#include <sys/ngc/p_prim.h>
#include <dolphin\dtk.h>
#include <dolphin/mix.h>
#include <dolphin.h>
#include "sys\ngc\p_aram.h"
#include <gel/mainloop.h>
#include	<sys/ngc\p_dvd.h>
#include <gel/soundfx/soundfx.h>
#include <gel/music/music.h>
#include	<sys/ngc\p_render.h>
#include	<sys/ngc\p_display.h>
#include	<sys/ngc\p_hw.h>
#include "sys/ngc/p_prim.h"
#include "gel\music\ngc\p_music.h"
#include "gfx/ngc/nx/nx_init.h"
#include <gfx\ngc\nx\texture.h>
#include <sys\ngc\p_gx.h>
#include <gfx\ngc\nx\render.h>
#include <gel/Scripting/script.h>

#include "VIDSimpleDEMO.h"
#include "VIDSimplePlayer.h"
#include "VIDSimpleAudio.h"
#include "VIDSimpleDraw.h"

#include <charpipeline/GQRSetup.h>
#include <sys/sioman.h>

#define MY_DEBUG

extern PADStatus		padData[PAD_MAX_CONTROLLERS]; // game pad state

extern GXColor messageColor;

#undef ASSERT
#define ASSERT(exp)                                             \
    (void) ((exp) ||                                            \
            (OSPanic(__FILE__, __LINE__, "Failed assertion " #exp), 0))

extern GXRenderModeObj *rmode;

//#define USE_DIRECT_XFB

//! Base address for 'Locked Cache' simple memory manager
static u8* 	lcMemBase = 0;

//! Locked cache base address for XFB conversion stuff
//! This is only required, if USE_DIRECT_XFB is set
static void* xfbLCStart = 0;		

//#define	VIDEO_FILENAME		"movies/peacemaker.vid"
//#define	VIDEO_FILENAME		"movies/video3.vid"
//#define	VIDEO_FILENAME		"movies/doomraiders.vid"
//#define	VIDEO_FILENAME		"movies/pu.vid"
//#define	VIDEO_FILENAME		"movies/nslogo.vid"
//#define	VIDEO_FILENAME		"movies/out4.vid"
//#define	VIDEO_FILENAME		"movies/hom.vid"
//#define	VIDEO_FILENAME		"movies/out5.vid"

extern bool	g_legal;

/******************************************************************************
 *  GLOABL VARIABLES
 ******************************************************************************/
extern void*   hwCurrentBuffer;         // current XFB frame buffer allocated in DEMO library 
VidSimplePlayer player;

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? a : b)
#endif

/******************************************************************************
 *  LOCAL VARIABLES
 ******************************************************************************/
static VidChunk workChunk ATTRIBUTE_ALIGN(32);
static void dvdDoneCallback(s32 result, DVDFileInfo *videoInfo);

/*!
 ******************************************************************************
 * \brief
 *		Initializes all player structures.
 *
 ******************************************************************************
 */
void VIDSimpleInit(VIDAllocator _cbAlloc, VIDDeallocator _cbFree, VIDAllocator _cbLockedCache)
	{
	memset(&player, 0, sizeof(player));
	player.cbAlloc = _cbAlloc;
	player.cbFree = _cbFree;
	player.cbLockedCache = _cbLockedCache;
	}

/*!
 ******************************************************************************
 * \brief
 *		Request an async file transfer.
 *
 *		This function starts the transfer of the next frame into a free
 *		buffer.
 *
 ******************************************************************************
 */
static void ReadFrameAsync(void)
	{
	if (!player.error && player.preFetchState == TRUE)
		{
		if (player.currentFrameCount >  player.videoInfo.maxFrameCount - 1)
			{
			if (player.loopMode)
				{
				player.currentFrameCount = 0;
				player.nextFrameOffset = player.firstFrameOffset;
				player.nextFrameSize  = player.firstFrameSize;
				}
			else
				return;
			}
		
		player.asyncDvdRunning = TRUE;

		if (DVDReadAsync(&player.fileHandle,
						 player.readBuffer[player.readIndex].ptr,
						 (s32)player.nextFrameSize,
						 (s32)player.nextFrameOffset, dvdDoneCallback) != TRUE )
			{
			player.asyncDvdRunning = FALSE;
			player.error = TRUE;
			}
		}
	}

/*!
 ******************************************************************************
 * \brief
 *		DVD callback if async transfer is finished (or aborted)
 *	
 *		The idea here is to transfer ONE frame and additional 32 bytes for the
 *		HEADER of the NEXT frame in one transfer step. We store the size of
 *		the next frame, which is used in ReadFrameAsync().
 *
 *		
 * \note
 *		There a 32 padding bytes at the end of the .vid file. So, the reading
 *		of 32 additional bytes is even possible for the LAST frame. (Otherwise,
 *		we would 'point' out of the file)
 *
 *		See Dolphin documentation for information about parameters.
 *
 ******************************************************************************
 */
static void dvdDoneCallback(s32 result, DVDFileInfo * _UNUSED(videoInfo))
	{
	if (result == DVD_RESULT_FATAL_ERROR)
		{
		player.error = TRUE;
		return;
		}
	else if (result == DVD_RESULT_CANCELED)
		{
		return;
		}

	player.asyncDvdRunning = FALSE;

	player.readBuffer[player.readIndex].frameNumber = player.currentFrameCount;
	player.readBuffer[player.readIndex].size = (u32)result;	
	player.readBuffer[player.readIndex].valid = TRUE;
	
	player.currentFrameCount++;

	// move file pointer
	player.nextFrameOffset += player.nextFrameSize;
	
	if(player.currentFrameCount < player.videoInfo.maxFrameCount)
		{
		// set read size for next 'FRAM' chunk
		u32* nextFrameStart = (u32*)(player.readBuffer[player.readIndex].ptr + player.nextFrameSize - 32);
	
		// some check if file structure is okay
		ASSERT(nextFrameStart[0] == VID_FCC('F','R','A','M'));
		
		// get the size of the next 'FRAM' chunk to read
		player.nextFrameSize = nextFrameStart[1];
		ASSERT(player.nextFrameSize);
		}
	else
		player.nextFrameSize = 0;	// at end of file we have a size of '0'. This should be reinitialized later
									// using the size of the first frame somwhere else! Otherwise, we get an assertion

	// use next buffer
	player.readIndex = (player.readIndex + 1) % VID_NUM_READ_BUFFERS;

	// continue loading if we have a free buffer
	if (!player.readBuffer[player.readIndex].valid)
		ReadFrameAsync();
	}

/*!
 ******************************************************************************
 * \brief
 *		Allocate buffer memory for asynchronous dvd read
 *
 * \param memAlloc
 *		Pointer to memory allocation function
 *
 * \return
 *		TRUE if DVD buffer setup was successful.
 *
 ******************************************************************************
 */
BOOL VIDSimpleAllocDVDBuffers(void)
	{
	u32 i;
	u32 bufferSize;
	u8* ptr;

	bufferSize = player.videoInfo.maxBufferSize;
	ASSERT(bufferSize);
	
	bufferSize += VID_CHUNK_HEADER_SIZE;	// 'fram' header
	bufferSize += VID_CHUNK_HEADER_SIZE;	// 'vidd' header
	bufferSize = OSRoundUp32B(bufferSize);
	
	ASSERT(player.cbAlloc);
	player.readBufferBaseMem = (u8*)((*player.cbAlloc)(bufferSize * VID_NUM_READ_BUFFERS));
	
	if(!player.readBufferBaseMem)
		return FALSE;	// out of mem
	
	ptr = player.readBufferBaseMem;
	for (i = 0; i < VID_NUM_READ_BUFFERS ; i++)
		{
		player.readBuffer[i].ptr = ptr;
		ptr += bufferSize;
		player.readBuffer[i].valid = FALSE;
		}
	
	return TRUE;
	}

/*!
 ******************************************************************************
 * \brief
 *		Free buffer memory used for dvd read
 *
 * \param memFree
 *		Pointer to memory deallocation function
 *
 ******************************************************************************
 */
void VIDSimpleFreeDVDBuffers(void)
	{
	ASSERT(player.cbFree);
	ASSERT(player.readBufferBaseMem);
	(*player.cbFree)(player.readBufferBaseMem);
	}

/*!
 ******************************************************************************
 * \brief
 *		Create a new decoder instance.
 *
 *		The required parameters about the decoding process will be supplied
 *		in the VIDDecoderSetup structure.
 *
 * \param supportBFrames
 *		Set to TRUE for enabling b-frame support.
 *
 * \return
 *		TRUE if decoder creation was successful
 *
 ******************************************************************************
 */
BOOL VIDSimpleCreateDecoder(BOOL supportBFrames)
	{
	VIDDecoderSetup	setup;
	
	setup.size = sizeof(VIDDecoderSetup);
	setup.width = player.videoInfo.width;
	setup.height = player.videoInfo.height;
	setup.flags = supportBFrames ? VID_DECODER_B_FRAMES : 0;
	setup.cbMemAlloc = player.cbAlloc;
	setup.cbMemFree = player.cbFree;
	setup.cbMemAllocLockedCache = player.cbLockedCache;

	// Check if we want to setup audio decoding.
	// The audio header info must be already preset here!
	if(player.audioInfo.audioID == VID_FCC('V','A','U','D'))
		{
		u32 skip;
		ASSERT(player.audioHeaderChunk);
		ASSERT(player.audioInfo.vaud.maxHeap > 0);
		ASSERT(player.audioInfo.vaud.preAlloc > 0);
		
		setup.flags |= VID_DECODER_AUDIO;

		skip = VID_CHUNK_HEADER_SIZE + sizeof(u32) + (player.audioInfo.vaudex.version > 0 ? player.audioInfo.vaudex.size : sizeof(VidAUDHVAUD));
		setup.audio.headerInfo = player.audioHeaderChunk + skip;
        setup.audio.headerInfoBytes = ((VidChunk*)player.audioHeaderChunk)->len - skip;
		setup.audio.maxHeap = player.audioInfo.vaud.maxHeap;
		setup.audio.preAlloc = player.audioInfo.vaud.preAlloc;
		}
	
	player.decoder = VIDCreateDecoder(&setup);

	// check if decoder creation failed!
	return player.decoder ? TRUE : FALSE;
	}/*!
 ******************************************************************************
 * \brief
 *		Destroy decoder instance.
 *
 *		At this point the decoder returns all allocated memory by using
 *		the cbFree callback.
 *
 ******************************************************************************
 */
void VIDSimpleDestroyDecoder(void)
	{
	ASSERT(player.decoder);
	VIDDestroyDecoder(player.decoder);
	}

/*!
 ******************************************************************************
 * \brief
 *		Preload the allocated buffers.
 *
 *		This functions fills all buffers with initial data
 *
 * \param loopMode
 *		TRUE if we want to operate in loop mode
 *
 * \return
 *		TRUE if preload was okay 		
 *
 ******************************************************************************
 */
BOOL VIDSimpleLoadStart(BOOL loopMode)
	{
	u8* ptr;
	u32	i, readNum;
	u32* nextFrame;

	if (player.open && player.preFetchState == FALSE)
		{
		
		readNum = VID_NUM_READ_BUFFERS;

		// in 'non-loop' mode we must take care if we have LESS frames than preloading buffers
        if (!loopMode && player.videoInfo.maxFrameCount < VID_NUM_READ_BUFFERS)
                readNum = player.videoInfo.maxFrameCount;
				
		for(i = 0; i < readNum; i++)
			{
			
			ptr = player.readBuffer[player.readIndex].ptr;
			
			// read total 'FRAM' chunk and 32 bytes of NEXT chunk
			if (DVDRead(&player.fileHandle, ptr, (s32)player.nextFrameSize, (s32)player.nextFrameOffset) < 0 )
				{
#ifdef MY_DEBUG
				OSReport("*** VIDSimpleLoadStart: Failed to read from file.\n");
#endif
				player.error = TRUE;
				return FALSE;
				}
			
			player.nextFrameOffset += player.nextFrameSize;
			player.readBuffer[player.readIndex].size = player.nextFrameSize;

            // set read size for next 'FRAM' chunk
			nextFrame = (u32*)(ptr + player.nextFrameSize - 32);
			
			// some sanity check if file structure is valid!
			ASSERT(nextFrame[0] == VID_FCC('F','R','A','M'));
			
			player.nextFrameSize = nextFrame[1];
			ASSERT(player.nextFrameSize);

			player.readBuffer[player.readIndex].valid = TRUE;
			player.readBuffer[player.readIndex].frameNumber = player.currentFrameCount;

			// use next buffer
			player.readIndex = (player.readIndex + 1) % VID_NUM_READ_BUFFERS;

			player.currentFrameCount++;

			if (player.currentFrameCount >  player.videoInfo.maxFrameCount - 1)
				{
				if (loopMode)
					{
					player.currentFrameCount = 0;
					player.nextFrameOffset = player.firstFrameOffset;
					player.nextFrameSize  = player.firstFrameSize;
					}
				}
			}
			
			player.loopMode = loopMode;
			player.preFetchState = TRUE;

			return TRUE;
		}
	return FALSE;
	}

/*!
 ******************************************************************************
 * \brief
 *		Stops the asynchronous loading process.
 *
 * \return
 *		TRUE if player could be stopped!		
 *
 ******************************************************************************
 */
BOOL VIDSimpleLoadStop(void)
	{
	u32 i;

	if (player.open)
		{
		// stop preloading process
		player.preFetchState = FALSE;

		if (player.asyncDvdRunning)
			{
			DVDCancel(&player.fileHandle.cb);
			player.asyncDvdRunning = FALSE;
			}

		// invalidate all buffers
		for (i = 0 ; i < VID_NUM_READ_BUFFERS; i++)
			player.readBuffer[i].valid = FALSE;

		player.nextFrameOffset = player.firstFrameOffset;
		player.nextFrameSize = player.firstFrameSize;
		player.currentFrameCount = 0;
		
		player.error 	   		= FALSE;
		player.readIndex   		= 0;
		player.decodeIndex 		= 0;

		return TRUE;
		}
	return FALSE;
    }

/*!
 ******************************************************************************
 * \brief
 *		Open video file.
 *
 *		This functions opens a video file and parses some basic file
 *		information.
 *
 * \param fileName
 *		Name of file to open
 *
 * \return
 *		TRUE if file could be opened and is in valid format!
 *
 ******************************************************************************
 */
BOOL VIDSimpleOpen(char* fileName, BOOL suppressAudio)
	{
	u32 fileOffset = 0;
	u32 headSize;
	u32 audioInfoSize;
	
    if (player.open)
		{
#ifdef _DEBUG
		OSReport("*** Cannot open '%s', because player already open.\n");
#endif
		return FALSE;
		}

	if (DVDOpen(fileName, &player.fileHandle) == FALSE)
		{
#ifdef _DEBUG
		OSReport("*** Cannot open: '%s'\n", fileName);
#endif
		return FALSE;
		}
		
	// Read 'VID1' chunk from file and check for correct version
    if (DVDRead(&player.fileHandle, &workChunk, 32, 0) < 0)
		{
#ifdef _DEBUG
		OSReport("*** Failed to read the header.\n");
#endif
		DVDClose(&player.fileHandle);
		return FALSE;
		}
		
	fileOffset += 32;
	
	// Check file id	
	if(workChunk.id != VID_FCC('V','I','D','1')  )
		{
#ifdef _DEBUG
		OSReport("*** No VID1 file: '%s'\n", fileName);
#endif
		DVDClose(&player.fileHandle);
		return FALSE;
		}
	
	// Check for correct version of vid chunk.
	// If we find this version we assume a 'special' alignment and chunk ordering which may be invalid
	// in another version of the file format.
	if(workChunk.vid.versionMajor != 1 || workChunk.vid.versionMinor != 0)
		{
#ifdef _DEBUG
		OSReport("*** Unsupported file version: major: %d, minor: %d\n",
				  workChunk.vid.versionMajor, workChunk.vid.versionMajor);
#endif
		DVDClose(&player.fileHandle);
		return FALSE;
		}
	
#ifdef _DEBUG
	// Sometimes, it's required to check for a special build of the VidConv converter.
	{
	u32 version = VID_VERSION(workChunk.vid.vidConvMajor, workChunk.vid.vidConvMinor, workChunk.vid.vidConvBuild);
	if(version < VID_VERSION(1,6,4))
		OSReport("*** WARNING: Vid file created using an unsupported converter version: %d.%d.%d\n", (u32)workChunk.vid.vidConvMajor, (u32)workChunk.vid.vidConvMinor, (u32)workChunk.vid.vidConvBuild);
	}
#endif

	// Check types of chunks we have in this file.
	// !!! Note that we assume start of 'HEAD' chunk at byte offset 32 from file start !!!
	if (DVDRead(&player.fileHandle, &workChunk, 32, (s32)fileOffset) < 0)
		{
#ifdef _DEBUG
		OSReport("*** Failed to read 'HEAD' chunk.\n");
#endif
		DVDClose(&player.fileHandle);
		return FALSE;
		}
	
	if(workChunk.id != VID_FCC('H','E','A','D')  )
		{
#ifdef _DEBUG
		OSReport("*** No HEAD chunk found at expected offset\n");
#endif
		DVDClose(&player.fileHandle);
		return FALSE;
		}
	
	// Calculate the start of the first frame chunk
	// (we know the header chunk starts at offset 32)
	player.nextFrameOffset = workChunk.len + 32;

	// Skip 'HEAD' chunk id, len and version fields
	fileOffset += VID_CHUNK_HEADER_SIZE;
	
	// We initialize audio codec info to a known value
	// (this way we can detect the absence of any audio data)
	player.audioInfo.audioID = VID_FCC('N','O','N','E');
	
	// The header chunk contains one or more header chunks for the different data types contained
	// in the stream. Parse them all...

	headSize = workChunk.len - VID_CHUNK_HEADER_SIZE;
	while(headSize >= 32)
		{
		if (DVDRead(&player.fileHandle, &workChunk, 32, (s32)fileOffset) < 0)
			{
#ifdef _DEBUG
			OSReport("*** Error reading file at offset %d\n", fileOffset);
#endif
			DVDClose(&player.fileHandle);
			return FALSE;
			}
		
		fileOffset += 32;
		headSize -= 32;

		// We analyze the 1st 32 bytes of the chunk for a known header format
		
		// Video header?
		if(workChunk.id == VID_FCC('V','I','D','H') )
			{
			// check if we have an old vid file.
			if(workChunk.version == 0)
				{
				workChunk.vidh.frameRateScale = (u16)(*((u32*)&workChunk.vidh.frameRateScale));
				workChunk.vidh.flags = 0;
				}
			
			// Yes...
			ASSERT(workChunk.len <= 32);
			ASSERT(workChunk.len <= (sizeof(VidVIDH) + VID_CHUNK_HEADER_SIZE));
			memcpy(&player.videoInfo, &workChunk.vidh, sizeof(VidVIDH));
			}
		// It's an audio header chunk! May we initialize it?
		else if(workChunk.id == VID_FCC('A','U','D','H') && !suppressAudio)
			{
			// Allocate memory for audio header chunk
            player.audioHeaderChunk = (u8*)((*player.cbAlloc)(workChunk.len));
			audioInfoSize = workChunk.len - VID_CHUNK_HEADER_SIZE;
			
			// Copy the already loaded part
			memcpy(player.audioHeaderChunk, &workChunk, 32);
			workChunk.len -= 32;
			
			// Read additional audio header bytes if the audio header is greater that 32 bytes
			if(workChunk.len >= 32)
				{
				ASSERT((workChunk.len&31)==0);
				if (DVDRead(&player.fileHandle, player.audioHeaderChunk+32, workChunk.len, (s32)fileOffset) < 0)
					{
#ifdef _DEBUG
					OSReport("*** Error reading file at offset %d\n", fileOffset);
#endif
					DVDClose(&player.fileHandle);
					return FALSE;
					}
				fileOffset += workChunk.len;
				headSize -= workChunk.len;
				}

			// Setup and calc the number of bytes which we are allowed to copy into the audioInfo struct
			memcpy(&player.audioInfo, player.audioHeaderChunk+VID_CHUNK_HEADER_SIZE, MIN(audioInfoSize, sizeof(player.audioInfo) + sizeof(player.adpcmInfo)));
			}
		// Skip unknown chunks. We already read 32 bytes for the header which we must subtract here.
		else
			{
			fileOffset += workChunk.len - 32;
			headSize -= workChunk.len - 32;
			}
        }
	
	ASSERT(player.videoInfo.width && player.videoInfo.height);
	ASSERT(player.videoInfo.maxBufferSize);
	
	player.fps = (f32)player.videoInfo.frameRate / (f32)player.videoInfo.frameRateScale;
	
	// read beginning of 1st frame chunk to get required size information
	if (DVDRead(&player.fileHandle, &workChunk, 32 , (s32)player.nextFrameOffset) < 0 )
		{
#ifdef _DEBUG
		OSReport("*** Failed to read 'FRAM' chunk.\n");
#endif
		DVDClose(&player.fileHandle);
		return FALSE;
		}

	if(workChunk.id != VID_FCC('F','R','A','M')  )
		{
#ifdef _DEBUG
		OSReport("*** No FRAM chunk found.");
#endif
		DVDClose(&player.fileHandle);
		return FALSE;
		}

	player.nextFrameSize = workChunk.len; 		// 32 bytes of this chunk are already consumed, but
													// we want to 'preload' the NEXT chunk's FRAM header
	player.nextFrameOffset += 32;
	
	player.firstFrameOffset = player.nextFrameOffset;
	player.firstFrameSize   = player.nextFrameSize;

	strncpy(player.fileName, fileName, 64);
	player.fileName[63] = 0;
	
	player.open 			 	= TRUE;

	player.readIndex 			= 0;
	player.decodeIndex 			= 0;
	player.lastDecodedFrame		= 0;
	player.error 			 	= FALSE;
	player.preFetchState 	 	= FALSE;
	player.loopMode			 	= FALSE;
	player.asyncDvdRunning		= FALSE;
	player.currentFrameCount 	= 0;
	player.readBufferBaseMem	= 0;

	return TRUE;
	} 	

/*!
 ******************************************************************************
 * \brief
 *		Close open video file
 *
 * \return
 *		TRUE if file is closed sucessfully.
 *
 ******************************************************************************
 */
BOOL VIDSimpleClose(void)
	{
	if (player.open)
		{
		if (player.preFetchState == FALSE)
			{
			if (!player.asyncDvdRunning)
				{
				player.open = FALSE;
				DVDClose(&player.fileHandle);
				if(player.audioHeaderChunk != NULL)
					{
					(*player.cbFree)(player.audioHeaderChunk);
					player.audioHeaderChunk = NULL;
					}
				return TRUE;
				}
			}
		}
	return FALSE;
	}

/*!
 ******************************************************************************
 * \brief
 *		Decode all frame data
 *
 *		This function operates on the full frame input data. It forwards this
 *		data to the required decoder.
 *
 ******************************************************************************
 */
BOOL VIDSimpleDecode(void)
	{
	BOOL enabled;
	u8* chunkStart;
	u32 chunkSize;
	u32 frameSize;

	if ( player.readBuffer[player.decodeIndex].valid )
		{
		
		// ptr to our (pre-) loaded data INSIDE (!) 'FRAM' chunk
		// (in other words, the 'FRAM' chunk itself is not visible here)
		chunkStart = player.readBuffer[player.decodeIndex].ptr;

		// usually, we read additional 32 bytes for getting info about the NEXT chunk.
		// We only deal with the actual 'FRAM' chunk data here and adjust the size by 32 bytes.
        frameSize = player.readBuffer[player.decodeIndex].size - 32;
		
		// loop across ALL chunks inside 'FRAM'
		while(frameSize >= 32)
			{
			chunkSize = VID_CHUNK_LEN(chunkStart);
			
			if( VID_CHUNK_ID(chunkStart) == VID_FCC('V','I','D','D') )
				{
				if(! VIDVideoDecode(player.decoder, chunkStart + VID_CHUNK_HEADER_SIZE, chunkSize - VID_CHUNK_HEADER_SIZE, &player.image))
					{
#ifdef _DEBUG
					OSReport("*** VIDVideoDecode failed!\n");
#endif
					}
				}
			else if( VID_CHUNK_ID(chunkStart) == VID_FCC('A','U','D','D') )
				{
				// This is audio data!
				
				// Get the data to the audio system...
				if(! VIDSimpleAudioDecode(chunkStart + VID_CHUNK_HEADER_SIZE, chunkSize - VID_CHUNK_HEADER_SIZE))
					{
#ifdef _DEBUG
					OSReport("*** VIDAudioDecode failed!\n");
#endif
					}
				}
#ifdef _DEBUG
			else
				{
				OSReport("*** VIDSimpleDecode: unknown chunk type!\n");
				}
#endif
			
			// goto next chunk
			chunkStart += chunkSize;
			frameSize -= chunkSize;
			}
			
		player.lastDecodedFrame = player.readBuffer[player.decodeIndex].frameNumber;
		player.readBuffer[player.decodeIndex].valid = FALSE;
		player.decodeIndex = (player.decodeIndex + 1) % VID_NUM_READ_BUFFERS;

		// check if loading is still running
		enabled = OSDisableInterrupts();
		if (!player.readBuffer[player.readIndex].valid && !player.asyncDvdRunning)
			ReadFrameAsync();
		OSRestoreInterrupts(enabled);
		
        return TRUE;
		}

#ifdef _DEBUG
	OSReport("*** VIDSimpleDecode: No valid decode buffer found (?).\n");
#endif
	return FALSE;

	}
/*!
 ******************************************************************************
 * \brief
 *		Draw a decoded video frame.
 *
 * \param rmode
 *		Required info about current rendering mode
 * \param x
 *		current x pos of drawing surface
 * \param y
 *		current y pos of drawing surface
 * \param width
 *		current width of drawing surface
 * \param height
 *		current height of drawing surface
 *
 ******************************************************************************
 */
void VIDSimpleDraw(GXRenderModeObj *rmode, u32 x, u32 y, u32 width, u32 height)
	{
	VIDDrawGXYuv2RgbSetup(rmode);
	VIDDrawGXYuv2RgbDraw((s16)x, (s16)y, (s16)width, (s16)height, player.image);
	VIDDrawGXRestore();
	}
/*!
 ******************************************************************************
 * \brief
 *		Draw decoded frame directely into the XFB
 *
 * \param rmode
 *		Required info about current rendering mode
 * \param lcMem
 *		Pointer to free locked cache mem used for conversion.
 *
 ******************************************************************************
 */
void VIDSimpleDrawXFB(GXRenderModeObj *rmode, void* lcMem)
	{
	VIDXFBDraw(player.image, hwCurrentBuffer, rmode->fbWidth, rmode->xfbHeight, lcMem);
	}

/*!
 ******************************************************************************
 * \brief
 *		Get width and height of loaded video file.
 *
 ******************************************************************************
 */
void VIDSimpleGetVideoSize(u32* width, u32* height)
	{
	// can only be returned if player has a loaded file
	ASSERT(player.open);
	
	*width = player.videoInfo.width;
	*height = player.videoInfo.height;
	}
	
/*!
 ******************************************************************************
 * \brief
 *		Get FPS rate of loaded video file.
 *
 ******************************************************************************
 */
f32 VIDSimpleGetFPS(void)
	{
	return(player.fps);
	}
/*!
 ******************************************************************************
 * \brief
 *		Check if the currently loaded video is in interlace mode or not
 *
 ******************************************************************************
 */
BOOL VIDSimpleIsInterlace(void)
	{
	return(((player.videoInfo.flags & VID_VIDH_INTERLACED) != 0));
	}

/*!
 ******************************************************************************
 * \brief
 *		Get number of frames of loaded video file.
 *
 ******************************************************************************
 */
u32 VIDSimpleGetFrameCount(void)
	{
	return(player.videoInfo.maxFrameCount);
	}

/*!
 ******************************************************************************
 * \brief
 *		Get audio sample rate in Hz.
 *
 ******************************************************************************
 */
u32 VIDSimpleGetAudioSampleRate(void)
	{
	return(player.audioInfo.vaud.frq);
	}
/*!
 ******************************************************************************
 * \brief
 *		Check for drive status
 *
 ******************************************************************************
 */
BOOL VIDSimpleCheckDVDError(void)
	{
	switch (DVDGetDriveStatus())
		{
		case DVD_STATE_FATAL_ERROR:
			{
			OSReport("DVDGetDriveStatus()=DVD_STATE_FATAL_ERROR\n");
			break;
			}
		case DVD_STATE_NO_DISK:
			{
			OSReport("DVDGetDriveStatus()=DVD_STATE_NO_DISK\n");
			break;
			}
		case DVD_STATE_COVER_OPEN:
			{
			OSReport("DVDGetDriveStatus()=DVD_STATE_COVER_OPEN\n");
			break;
			}
		case DVD_STATE_WRONG_DISK:
			{
			OSReport("DVDGetDriveStatus()=DVD_STATE_WRONG_DISK\n");
			break;
			}
		case DVD_STATE_RETRY:
			{
			OSReport("DVDGetDriveStatus()=DVD_STATE_RETRY\n");
			break;
			}
		default:
			return(TRUE);
		}
	
	return(FALSE);
	}

/*!
 ******************************************************************************
 * \brief
 *		Memory allocation callback.
 *
 *		The player calls this function for all its memory needs. In this example
 *		an OSAlloc() is all we need to do.
 *
 * \note
 *		The returned memory address MUST be aligned on a 32 byte boundary.
 *		Otherwise, the player will fail!
 *
 * \param size
 *		Number of bytes to allocate
 *
 * \return
 *		Ptr to allocated memory (aligned to 32 byte boundary)
 *	
 ******************************************************************************
 */
static void* myAlloc(u32 size)
{
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
	void * p = new u8[size]; 
	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
	Mem::Manager::sHandle().PopContext();
	return p;
}

/*!
 ******************************************************************************
 * \brief
 *		Memory free callback.
 *
 * \param ptr
 *		Memory address to free
 *	
 ******************************************************************************
 */
static void myFree(void* ptr)
	{
	ASSERT(ptr);	// free on address 0 is only valid in C++ delete
	delete ptr;
	}

/*!
 ******************************************************************************
 * \brief
 *		Memory allocation callback for 'Locked Cache' memory.
 *
 *		If the system should operate in 'Locked Cache' mode, you must
 *		supply a callback which is called for any 'locked cache'
 *		memory requirements.
 *
 *		Note that there's no 'free' for the 'locked cache' memory,
 *		because if the player is destroyed ANY 'locked cache' memory is
 *		avaiable immediately.
 *
 * \note
 *		The returned memory address MUST be aligned on a 32 byte boundary.
 *		Otherwise, the player will fail!
 *
 * \param size
 *		Number of bytes to allocate
 *
 * \return
 *		Ptr to allocated memory (aligned to 32 byte boundary)
 *	
 ******************************************************************************
 */
static void* myAllocFromLC(u32 size)
	{
#ifdef MY_DEBUG
	u32 lockCacheMem;
#endif
	void* ret = lcMemBase;
	ASSERT(ret);
	
	lcMemBase += size;
	lcMemBase = (u8*)OSRoundUp32B(lcMemBase);

#ifdef MY_DEBUG
	lockCacheMem = (u32)(lcMemBase - ((u8*)LCGetBase()));
	//OSReport("myMallocFromLC: Used 'Locked Cache' Mem: %d kB.\n", lockCacheMem/1024);
	ASSERTMSG(lockCacheMem < (15*1024), "myMallocFromLC: Too much locked cache mem needed!\n");
#endif
	return ret;
	}

/******************************************************************************
 *  DEFINES
 ******************************************************************************/

/*!
 ******************************************************************************
 * \brief
 *		Restore GX graphics context to some 'defaults'
 *
 ******************************************************************************
 */
void VIDDrawGXRestore(void)
	{
    GXSetZMode(GX_ENABLE, GX_ALWAYS, GX_DISABLE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);

    GXSetNumTexGens(1);
    GXSetNumChans(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);

    // Swap mode settings
    GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevSwapMode(GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevSwapMode(GX_TEVSTAGE2, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetTevSwapMode(GX_TEVSTAGE3, GX_TEV_SWAP0, GX_TEV_SWAP0);

    GXSetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED,   GX_CH_GREEN,
                                        GX_CH_BLUE,  GX_CH_ALPHA); // RGBA
    GXSetTevSwapModeTable(GX_TEV_SWAP1, GX_CH_RED,   GX_CH_RED,
                                        GX_CH_RED,   GX_CH_ALPHA); // RRRA
    GXSetTevSwapModeTable(GX_TEV_SWAP2, GX_CH_GREEN, GX_CH_GREEN,
                                        GX_CH_GREEN, GX_CH_ALPHA); // GGGA
    GXSetTevSwapModeTable(GX_TEV_SWAP3, GX_CH_BLUE,  GX_CH_BLUE,
                                        GX_CH_BLUE,  GX_CH_ALPHA); // BBBA

	}

/*!
 ******************************************************************************
 * \brief
 *		Setup GX for YUV conversion
 *
 ******************************************************************************
 */
void VIDDrawGXYuv2RgbSetup(GXRenderModeObj *rmode)
	{
    s32         scrWidth;
    s32         scrHeight;
    Mtx44       pMtx;

    scrWidth  = rmode->fbWidth;
    scrHeight = rmode->efbHeight;

	GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	MTXOrtho(pMtx, 0.0f, (f32)scrHeight, 0.0f, (f32)scrWidth, 0.0f, -1.0F);

	GXSetProjection(pMtx, GX_ORTHOGRAPHIC);
    GXSetViewport(0.0F, 0.0F, (f32)scrWidth, (f32)scrHeight, 0.0F, 1.0F);
	GXSetScissor(0, 0, (u32)scrWidth, (u32)scrHeight);
	
    GXSetCurrentMtx(GX_IDENTITY);

    // Framebuffer
    GXSetZMode(GX_ENABLE, GX_ALWAYS, GX_DISABLE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetDispCopyGamma(GX_GM_1_0);
	
	// Color channels
    GXSetNumChans(0);

    // Texture coord generation
    GXSetNumTexGens(2);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY);

    // Texture cache
    GXInvalidateTexAll();

    // Vertex formats
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS,  GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX1, GX_DIRECT);
	
    GXSetVtxAttrFmt(GX_VTXFMT7, GX_VA_POS,  GX_POS_XYZ, GX_S16, 0);
    GXSetVtxAttrFmt(GX_VTXFMT7, GX_VA_TEX0, GX_TEX_ST,  GX_U16, 0);
    GXSetVtxAttrFmt(GX_VTXFMT7, GX_VA_TEX1, GX_TEX_ST,  GX_U16, 0);

	// Setup TEV environment to perform color space conversion.
	// The function will return the number of TEV stages need and will
	// use the following HW resources:
	//
	//	GX_TEVPREV
	//	GX_TEVREG0
	//	GX_TEVREG1
	//	GX_TEVREG2
	//
	//	GX_KCOLOR0
	//	GX_KCOLOR1
	//	GX_KCOLOR2
	//	GX_KCOLOR3
	//
	// plus everything visible in this source
	//
	GXSetNumTevStages(VIDSetupTEV(VID_YUVCONV_HIGHPRECISION));
	}

/*!
 ******************************************************************************
 * \brief
 *		Draw a textured polygon using the decoded image a texture.
 *
 * \param x
 *		xpos of polygon on screen
 * \param y
 *		ypos of polygon on screen
 * \param polygonWidth
 *		width of polygon to draw
 * \param polygonHeight
 *		height of polygon to draw
 * \param image
 *		ptr to VIDImage containing the required YUV pointers
 ******************************************************************************
 */
void VIDDrawGXYuv2RgbDraw(s16 x, s16 y, s16 polygonWidth, s16 polygonHeight, const VIDImage* image)
	{
	u16 textureWidth2, textureHeight2;
	GXTexObj tobj0, tobj1, tobj2;
	
	textureWidth2 = (u16)(image->texWidth >> 1);
	textureHeight2 = (u16)(image->texHeight >> 1);
	
	// Y Texture
	GXInitTexObj(&tobj0, image->y, image->texWidth, image->texHeight,
                 GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&tobj0, GX_LINEAR, GX_LINEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
	GXLoadTexObj(&tobj0, GX_TEXMAP0);

	// Cb Texture
	GXInitTexObj(&tobj1, image->u, textureWidth2, textureHeight2,
                 GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&tobj1, GX_LINEAR, GX_LINEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
	GXLoadTexObj(&tobj1, GX_TEXMAP1);

	// Cr Texture
	GXInitTexObj(&tobj2, image->v, textureWidth2, textureHeight2,
                 GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&tobj2, GX_LINEAR, GX_LINEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
	GXLoadTexObj(&tobj2, GX_TEXMAP2);

	GXSetTexCoordScaleManually(GX_TEXCOORD0, GX_ENABLE, 1, 1);
	GXSetTexCoordScaleManually(GX_TEXCOORD1, GX_ENABLE, 1, 1);

	// Draw a textured quad
	GXBegin(GX_QUADS, GX_VTXFMT7, 4);
		GXPosition3s16(x, y, 0);
		GXTexCoord2u16(0, 0);
		GXTexCoord2u16(0, 0);
		
		GXPosition3s16((s16)(x+polygonWidth), y, 0);
		GXTexCoord2u16(image->width, 0);
		GXTexCoord2u16((u16)(image->width>>1), 0);

		GXPosition3s16((s16)(x+polygonWidth), (s16)(y+polygonHeight), 0);
		GXTexCoord2u16(image->width, image->height);
		GXTexCoord2u16((u16)(image->width>>1), (u16)(image->height>>1));
		
		GXPosition3s16(x, (s16)(y+polygonHeight), 0);
		GXTexCoord2u16(0, image->height);
		GXTexCoord2u16(0, (u16)(image->height>>1));
	GXEnd();

	GXSetTexCoordScaleManually(GX_TEXCOORD0, GX_DISABLE, 1, 1);
	GXSetTexCoordScaleManually(GX_TEXCOORD1, GX_DISABLE, 1, 1);
	
	}

/******************************************************************************
 *  DEFINES
 ******************************************************************************/

#define	VID_AUDIO_READAHEADFRAMES	8			// Number of video frames worth of audio data that should be able to be stored prio to being routed into the AI buffer
												// (the data stream will contain MORE data than needed at times -- esspecially at the beginning of the stream.
												//  Hence an intermediate buffer is needed!)

#define	VID_AUDIO_NUMLEADFRAMES		4			// Number of lead in frames for audio in data file (VIDCONV default)
#define	VID_AUDIO_LEADFACTOR		1.5f		// Lead in data ratio factor in lead in frames (VIDCONV default)

#define	VID_AUDIO_AIBUFFERSAMPLES	(2*256)		// 10.6ms of 48KHz data per AI buffer (32 byte multiple), that'll be about 15.9ms at 32Khz
#define	VID_AUDIO_NUMAIBUFFERS		2			// Number of AI playback buffers (this has an impact on audio latency. 2 is the minimum needed)
#define VID_AUDIO_NUMAIREQUESTS		16

#define	AX_ARAM_BUFFER_SIZE			(VID_AUDIO_AIBUFFERSAMPLES * VID_AUDIO_NUMAIBUFFERS)

//#define	AX_ARAM_LEFT_CHANNEL		0x200000	// @ 4MB (16-Bit addressing for DSP!)
#define	AX_ARAM_LEFT_CHANNEL		( NxNgc::EngineGlobals.aram_stream0 >> 1 )
#define	AX_ARAM_RIGHT_CHANNEL		(AX_ARAM_LEFT_CHANNEL + AX_ARAM_BUFFER_SIZE)

/******************************************************************************
 *  LOCAL VARIABLES & LOCAL EXTERNAL REFERENCES
 ******************************************************************************/

extern VidSimplePlayer	player;										// Player instance

static void				*audioReadBuffer;							// Buffer to store audio data received from the data stream
static u32				audioReadBufferNumSamples;					// Size of the above buffer in samples
static u32				audioReadBufferWritePos;					// Write position in the above buffer in samples
static u32				audioReadBufferReadPos;						// Read position in the above buffer in samples
static u8				audioReadBufferNumChannels;					// Number of channels stored in the read buffer

static void				*audioPlayBuffer[VID_AUDIO_NUMAIBUFFERS];	// AI playback buffers
static u8				audioPlayBufferWriteIndex;					// Index to next AI buffer to be written to from the read buffer
static u32				audioPlayBufferFrq;							// Playback frequency of AI in Hz
static volatile BOOL	audioPlayBufferEnabled;						// TRUE if playback is enabled. AI will operate at all times, but zeros will be filled in instead of real data if this is FALSE.
static volatile BOOL	audioPlayBackPossible;						// TRUE if read buffer is initialized and playback may start
																	// Normally this should be 1. It should never be greater or equal to the number of AI buffers.

static void (*VIDSimpleDoAudioDecode)(s16* dest,u32 channels,const s16** samples,u32 sampleOffset,u32 sampleNum); // vector to the current decoder function
static u32 (*VIDSimpleAudioBytesFromSamples)(u32 samples);					// vector to the current bytes to samples conversion function
static u32 (*VIDSimpleAudioSamplesFromBytes)(u32 bytes);					// vector to the current samples to bytes conversion function

static const u32		*audioPlayMaskArray;						// Pointer to an array of channel play masks (each set bit signals an active channel)
static u32				audioNumPlayMasks;							// Number of play masks specified (0 indicates all channels should be active)
static u32				audioNumActiveVoices;						// Number of active voices

static AXVPB			*axVoice[2];								// AX voice structures
static ARQRequest		arqRequest[2][VID_AUDIO_NUMAIREQUESTS];		// Enough ARQ request structures for worst case scenario
static u32				axLastAddr;									// Last known address DSP read from for 1st voice
static u32				axPlayedSamples;							// Number of samples played on first voice since last update
static f32				axCurrentFrq;								// Current frequency used for playback
static u32				axMinAvailFrames;							// Minimum number of frames available in the read buffer at which we are still "happy" and play at the specified frequency			

typedef enum VID_AXPHASE {
		AX_PHASE_STARTUP = 0,
		AX_PHASE_START,
		AX_PHASE_PLAY
} VID_AXPHASE;

static VID_AXPHASE		axPhase;

/******************************************************************************
 *  LOCAL PROTOTYPES
 ******************************************************************************/

static void AXCallback(void);

static void VIDSimpleDoAudioDecodePCM16(s16* dest,u32 channels,const s16** samples,u32 sampleOffset,u32 sampleNum);
static u32 VIDSimpleAudioBytesFromSamplesPCM16(u32 samples);
static u32 VIDSimpleAudioSamplesFromBytesPCM16(u32 bytes);

static void VIDSimpleDoAudioDecodeVAUD(s16* dest,u32 channels,const s16** samples,u32 sampleOffset,u32 sampleNum);
static u32 VIDSimpleAudioBytesFromSamplesVAUD(u32 samples);
static u32 VIDSimpleAudioSamplesFromBytesVAUD(u32 bytes);

/*!
 ******************************************************************************
 * \brief
 *		Initialize audio decoder
 *
 *		This function allocates all neccessary memory for the audio processing
 *		and sets the audio decoder into an idle state, waiting for first data.
 *		A file must be opened with the VIDSimplePlayer before calling this
 *		function.
 *
 * \param cbAlloc
 *		Pointer to a memory allocation function to be used
 *
 * \return
 *		TRUE if any problem was detected
 *
 ******************************************************************************
 */
BOOL VIDSimpleInitAudioDecoder(void)
	{
	u32	i;
	BOOL old;

	AXPBMIX	  axMix[2];
	AXPBVE    axVE;
	AXPBSRC	  axSRC;
	AXPBADDR  axAddr;
	AXPBADPCM axADPCM;
	u32		  ratio;

	// Do we have any audio data?
	if (player.audioInfo.audioID != VID_FCC('N','O','N','E'))
		{
		// VAUD?
		if (player.audioInfo.audioID == VID_FCC('V','A','U','D'))
			{
			// Calculate buffer size to allocate proper memry to keep a bit of "extra" audio data around...
			audioReadBufferNumSamples = (u32)(((f32)VID_AUDIO_READAHEADFRAMES * player.audioInfo.vaud.frq) / player.fps);
			audioReadBufferNumChannels = player.audioInfo.vaud.numChannels <= 2 ? player.audioInfo.vaud.numChannels : 2;

			// Setup decoder & conversion functions to be used
			VIDSimpleDoAudioDecode = VIDSimpleDoAudioDecodeVAUD;
			VIDSimpleAudioBytesFromSamples = VIDSimpleAudioBytesFromSamplesVAUD;
			VIDSimpleAudioSamplesFromBytes = VIDSimpleAudioSamplesFromBytesVAUD;
			}
		else
			{
			// PCM16?
	        if (player.audioInfo.audioID == VID_FCC('P','C','1','6'))
				{
		        // Calculate buffer size to allocate proper memry to keep a bit of "extra" audio data around...
		        audioReadBufferNumSamples = (u32)(((f32)VID_AUDIO_READAHEADFRAMES * player.audioInfo.pcm16.frq) / player.fps);
        
		        if (player.audioInfo.pcm16.numChannels <= 2)
			        audioReadBufferNumChannels = player.audioInfo.pcm16.numChannels;
		        else
			        audioReadBufferNumChannels = 2;
		        
		        // Setup decoder & conversion functions to be used
		        VIDSimpleDoAudioDecode = VIDSimpleDoAudioDecodePCM16;
		        VIDSimpleAudioBytesFromSamples = VIDSimpleAudioBytesFromSamplesPCM16;
		        VIDSimpleAudioSamplesFromBytes = VIDSimpleAudioSamplesFromBytesPCM16;
		        }
	        else
		        {
		        if (player.audioInfo.audioID == VID_FCC('A','P','C','M'))
			        {
			        // [...]
			        }
		        else
			        {
			        // Other audio codecs might be implemented here
			        // (the idea being to decode the data into the audioReadBuffer allocated below)
			        // [...]
			        ASSERT(FALSE);
			        }
		        }
			}
			
		// Allocate read buffer
		audioReadBuffer = player.cbAlloc(audioReadBufferNumSamples * sizeof(s16) * player.audioInfo.pcm16.numChannels);
		if (audioReadBuffer == NULL)
			return TRUE;					// error
		
		// Reset ring buffer
		audioReadBufferReadPos = audioReadBufferWritePos = 0;
		
		// What AI frquency is best?
		audioPlayBufferFrq = player.audioInfo.pcm16.frq;
		
		// Allocate AI playback buffer
		audioPlayBuffer[0] = player.cbAlloc(2 * sizeof(s16) * VID_AUDIO_AIBUFFERSAMPLES * VID_AUDIO_NUMAIBUFFERS);
		if (audioPlayBuffer[0] == NULL)
			return TRUE;					// error
		
		for(i=1; i<VID_AUDIO_NUMAIBUFFERS; i++)
			audioPlayBuffer[i] = (void *)((u32)audioPlayBuffer[i - 1] + (2 * sizeof(s16) * VID_AUDIO_AIBUFFERSAMPLES));
		
		// Reset buffer index
		audioPlayBufferWriteIndex = 0;
		
		// We disable AI output for now (logically)
		audioPlayBufferEnabled = FALSE;
		audioPlayBackPossible = FALSE;
		
		// We assume to playback all we get by default
		audioPlayMaskArray = NULL;
		audioNumPlayMasks = 0;
		audioNumActiveVoices = 2;
		
		// Clear out AI buffers to avoid any noise what so ever
		memset(audioPlayBuffer[0],0,2 * sizeof(s16) * VID_AUDIO_AIBUFFERSAMPLES * VID_AUDIO_NUMAIBUFFERS);
		DCFlushRange(audioPlayBuffer[0],2 * sizeof(s16) * VID_AUDIO_AIBUFFERSAMPLES * VID_AUDIO_NUMAIBUFFERS);
		
		// Init GCN audio system
		old = OSDisableInterrupts();

		axVoice[0] = AXAcquireVoice(AX_PRIORITY_NODROP,NULL,0);
		ASSERT(axVoice[0] != NULL);
		axVoice[1] = AXAcquireVoice(AX_PRIORITY_NODROP,NULL,0);
		ASSERT(axVoice[1] != NULL);

		memset(&axMix[0],0,sizeof(axMix[0]));
		axMix[0].vL = 0x7FFF;
		memset(&axMix[1],0,sizeof(axMix[1]));
		axMix[1].vR = 0x7FFF;
		
		AXSetVoiceMix(axVoice[0],&axMix[0]);
		AXSetVoiceMix(axVoice[1],&axMix[1]);
		
		axVE.currentDelta = 0;
		axVE.currentVolume = 0x7FFF;
		AXSetVoiceVe(axVoice[0],&axVE);
		AXSetVoiceVe(axVoice[1],&axVE);
		
		memset(&axSRC,0,sizeof(AXPBSRC));
		
		ratio = (u32)(65536.0f * (f32)audioPlayBufferFrq / (f32)AX_IN_SAMPLES_PER_SEC);
		axSRC.ratioHi = (u16)(ratio >> 16);
		axSRC.ratioLo = (u16)ratio;
		
		axCurrentFrq = (f32)audioPlayBufferFrq;

		// Calculate what we deem is a save amount of extra data in our read buffers
		axMinAvailFrames = (u32)((audioPlayBufferFrq * (VID_AUDIO_LEADFACTOR - 1.0f) * ((f32)VID_AUDIO_NUMLEADFRAMES / player.fps)) / (f32)VID_AUDIO_AIBUFFERSAMPLES);
		
		AXSetVoiceSrcType(axVoice[0],AX_SRC_TYPE_4TAP_16K);
		AXSetVoiceSrc(axVoice[0],&axSRC);
		AXSetVoiceSrcType(axVoice[1],AX_SRC_TYPE_4TAP_16K);
		AXSetVoiceSrc(axVoice[1],&axSRC);
		
        *(u32 *)&axAddr.currentAddressHi = AX_ARAM_LEFT_CHANNEL;
		*(u32 *)&axAddr.loopAddressHi = AX_ARAM_LEFT_CHANNEL;
		*(u32 *)&axAddr.endAddressHi = AX_ARAM_LEFT_CHANNEL + AX_ARAM_BUFFER_SIZE - 1;
		axAddr.format = AX_PB_FORMAT_PCM16;
		axAddr.loopFlag = AXPBADDR_LOOP_ON;
		AXSetVoiceAddr(axVoice[0],&axAddr);
		
		*(u32 *)&axAddr.currentAddressHi = AX_ARAM_RIGHT_CHANNEL;
		*(u32 *)&axAddr.loopAddressHi = AX_ARAM_RIGHT_CHANNEL;
		*(u32 *)&axAddr.endAddressHi = AX_ARAM_RIGHT_CHANNEL + AX_ARAM_BUFFER_SIZE - 1;
		AXSetVoiceAddr(axVoice[1],&axAddr);

		memset(&axADPCM,0,sizeof(axADPCM));
		axADPCM.gain = 0x0800;
		
		AXSetVoiceAdpcm(axVoice[0],&axADPCM);
		AXSetVoiceAdpcm(axVoice[1],&axADPCM);
		
		AXSetVoiceType(axVoice[0],AX_PB_TYPE_STREAM);
		AXSetVoiceType(axVoice[1],AX_PB_TYPE_STREAM);
		
		AXRegisterCallback(AXCallback);
		
		axLastAddr = AX_ARAM_LEFT_CHANNEL;
		axPlayedSamples = VID_AUDIO_AIBUFFERSAMPLES * VID_AUDIO_NUMAIBUFFERS;
		axPhase = AX_PHASE_STARTUP;
		
		// All is setup for the voices. We'll start them inside the AX callback as soon as we got data in the ARAM buffers
        OSRestoreInterrupts(old);
		}
		
	return FALSE;			// no error
	}

/*!
 ******************************************************************************
 * \brief
 *		Shutdown audio decoder and free resources
 *
 * \param cbFree
 *		Pointer to a fucntion to be used to free the allocated memory
 *
 ******************************************************************************
 */
void VIDSimpleExitAudioDecoder(void)
	{
	// Any audio?
	if (player.audioInfo.audioID != VID_FCC('N','O','N','E'))
		{
		// Yes. Unregister callback & stop AI DMA
		BOOL old = OSDisableInterrupts();
		
		AXRegisterCallback(NULL);
		AXSetVoiceState(axVoice[0],AX_PB_STATE_STOP);
		AXSetVoiceState(axVoice[1],AX_PB_STATE_STOP);
		AXFreeVoice(axVoice[0]);
		AXFreeVoice(axVoice[1]);
		
		axVoice[0] = axVoice[1] = NULL;
		
		OSRestoreInterrupts(old);
		
		// Any codec related resources should be freed, too
		// [...]
		
		// Free allocated resources...
		player.cbFree(audioPlayBuffer[0]);
		player.cbFree(audioReadBuffer);
		}
	}


/*!
 ******************************************************************************
 * \brief
 *		Stop audio playback without shutting down AI etc.
 *
 ******************************************************************************
 */
void VIDSimpleAudioReset(void)
	{
	BOOL	old;
	
	if (player.audioInfo.audioID != VID_FCC('N','O','N','E'))
		{
		old = OSDisableInterrupts();
		
		audioReadBufferWritePos = 0;
		audioReadBufferReadPos = 0;
		
		audioPlayBufferEnabled = FALSE;
		audioPlayBackPossible = FALSE;
		
		// ADPCM?
		if (player.audioInfo.audioID != VID_FCC('A','P','C','M'))
			{
			// [...]
			}
		else
			{
			// Other codecs could reset their state right here...
			// [...]
			}
			
		OSRestoreInterrupts(old);
		}
	}

/*!
 ******************************************************************************
 * \brief
 *		Return some information about current audio stream.
 *
 ******************************************************************************
 */
void VIDSimpleAudioGetInfo(VidAUDH* audioHeader)
	{
	ASSERT(audioHeader);
	memcpy(audioHeader, &player.audioInfo, sizeof(*audioHeader));
    }

/*!
 ******************************************************************************
 * \brief
 *		'Decode' PCM16 to PCM16
 *
 *		This function simply copies over new PCM16 samples into the read buffer.
 *		Other codecs might use a more elaborate decoding of course ;-)
 *
 ******************************************************************************
 */
static void VIDSimpleDoAudioDecodePCM16(s16* dest, u32 channels, const s16** samples, u32 sampleOffset, u32 sampleNum)
	{
	u32		i,b,s;
    u32 		bytes  = VIDSimpleAudioBytesFromSamplesPCM16(sampleNum);
	const void*	src    = (const u8*)samples[0] + VIDSimpleAudioBytesFromSamplesPCM16(sampleOffset);
	
	// Do we have any playback masks?
	if (audioNumPlayMasks != 0)
		{
		// Yes! How many samples?
		u32 numSamples = bytes / (player.audioInfo.pcm16.numChannels * sizeof(s16));
		
		// Scan through all playback masks and update channels as they come
		u32 c = 0;
		u32 rc = 0;
		for(i=0; i<audioNumPlayMasks; i++)
			{
			for(b=0; b<32; b++)
				{
				if (audioPlayMaskArray[i] & (1<<b))
					{
					for(s=0; s<numSamples; s++)
						dest[s * audioReadBufferNumChannels + c] = ((s16 *)src)[s * player.audioInfo.pcm16.numChannels + rc];
					c++;
					
					// If we already updated all read buffer channels we may exit. There's no more space anyways...
					if (c == audioReadBufferNumChannels)
						return;
					}
				rc++;
				
				// If we already read from all channels in the source, we can exit, too...
				if (rc == player.audioInfo.pcm16.numChannels)
					return;
				}
			}
		}
	else
		{
		// We should use all channels we got. Okay, memcpy everything...
		memcpy(dest,src,bytes);
		}
	}


/*!
 ******************************************************************************
 * \brief
 *		Misc conversion fucntions for PCM16 stream data
 *
 ******************************************************************************
 */
static u32 VIDSimpleAudioBytesFromSamplesPCM16(u32 samples)
	{
	return(samples * sizeof(s16) * player.audioInfo.pcm16.numChannels);
	}

static u32 VIDSimpleAudioSamplesFromBytesPCM16(u32 bytes)
	{
	return(bytes / (sizeof(s16) * player.audioInfo.pcm16.numChannels));
	}

/*!
 ******************************************************************************
 * \brief
 *		Decode VAUD Data.
 *
 *		This function simply interleaves two channels from from the
 *		audio decoder output into the required format.
 *
 ******************************************************************************
 */
static void VIDSimpleDoAudioDecodeVAUD(s16* dest, u32 channels, const s16** samples, u32 sampleOffset, u32 sampleNum)
	{
	u32 j;
	if(channels == 1)
		{
		const s16* in = samples[0] + sampleOffset;
		for(j = 0; j < sampleNum; j++)
			{
			*dest++ = in[j];
			*dest++ = in[j];
			}
		}
	else
		{
		const s16* inL = samples[0] + sampleOffset;
		const s16* inR = samples[1] + sampleOffset;
		ASSERT(channels == 2);
		for(j = 0; j < sampleNum; j++)
			{
			*dest++ = *inL++;
			*dest++ = *inR++;
			}
		}
	}

/*!
 ******************************************************************************
 * \brief
 *		Misc conversion fucntions for VAUD stream data
 *
 ******************************************************************************
 */
static u32 VIDSimpleAudioBytesFromSamplesVAUD(u32 samples)
	{
	return(samples * sizeof(s16) * player.audioInfo.pcm16.numChannels);
	}

static u32 VIDSimpleAudioSamplesFromBytesVAUD(u32 bytes)
	{
	return(bytes / (sizeof(s16) * player.audioInfo.pcm16.numChannels));
	}

/*!
 ******************************************************************************
 * \brief
 *		Main audio data decode function.
 *
 *		This function will be used as a callback from the VIDAudioDecode
 *		function or is called directely in case of PCM or ADPCM.
 *
 * \param numChannels
 *		Number of channels present in the sample array
 *
 * \param samples
 *		Array of s16 pointers to the sample data 
 *
 * \param sampleNum
 *		Number of samples in the array. All arrays have the same amount
 *		of sample data
 *
 * \param userData
 *		Some user data
 *
 * \return
 *		FALSE if data could not be interpreted properly
 *
 ******************************************************************************
 */
static BOOL audioDecode(u32 numChannels, const s16 **samples, u32 sampleNum, void* userData)
	{
	u32		freeSamples;
	u32		sampleSize;
	u32		len1;
	BOOL	old;
	
	// we can only play mono or stereo!
	ASSERT(numChannels <= 2);

	// Disable IRQs. We must make sure we don't get interrupted by the AI callback.
	old = OSDisableInterrupts();

	// Did the video decoder just jump back to the beginning of the stream?
	if (player.readBuffer[player.decodeIndex].frameNumber < player.lastDecodedFrame)
		{
		// Yes! We have to reset the internal read buffer and disable any audio output
		// until we got new video data to display...
		//
		// Note: we have to reset our buffers because the stream contains more audio data than
		// neccessary to cover a single video frame within the first few frames to accumulate
		// some safety buffer. If the stream would just be allowed to loop we would get an
		// overflow (unless we used up the extra buffer due to read / decode delays) after a few
		// loops...
		//
		VIDSimpleAudioReset();
		}
	
	// Calculate the read buffer's sample size
	sampleSize = sizeof(s16) * audioReadBufferNumChannels;
	
	// How many samples could we put into the buffer?
	if (audioReadBufferWritePos >= audioReadBufferReadPos)
		{
		freeSamples = audioReadBufferNumSamples - (audioReadBufferWritePos - audioReadBufferReadPos);
		
		if (freeSamples < sampleNum)
			{
			OSRestoreInterrupts(old);
			#ifndef FINAL
			OSReport("*** audioDecode: overflow\n");
			#endif
			return FALSE;				// overflow!
			}
		
		// We might have a two buffer update to do. Check for it...
		if ((len1 = (audioReadBufferNumSamples - audioReadBufferWritePos)) >= sampleNum)
			{
			// No. We got ourselfs a nice, simple single buffer update.
			VIDSimpleDoAudioDecode((s16 *)((u32)audioReadBuffer + audioReadBufferWritePos * sampleSize),numChannels,samples,0,sampleNum);
			}
		else
			{
			// Dual buffer case
			VIDSimpleDoAudioDecode((s16 *)((u32)audioReadBuffer + audioReadBufferWritePos * sampleSize),numChannels,samples,0,len1);
			VIDSimpleDoAudioDecode((s16 *)audioReadBuffer,numChannels,samples,len1,sampleNum-len1);
			}
		}
	else
		{
		freeSamples = audioReadBufferReadPos - audioReadBufferWritePos;
		
		if (freeSamples < sampleNum)
			{
			OSRestoreInterrupts(old);
			#ifndef FINAL
			OSReport("*** audioDecode: overflow\n");
			#endif
			return FALSE;				// overflow!
			}
            // We're save to assume to have a single buffer update in any case...
			VIDSimpleDoAudioDecode((s16 *)((u32)audioReadBuffer + audioReadBufferWritePos * sampleSize),numChannels,samples,0,sampleNum);
		}

	// Advance write position...
	audioReadBufferWritePos += sampleNum;
		
	if (audioReadBufferWritePos >= audioReadBufferNumSamples)
		audioReadBufferWritePos -= audioReadBufferNumSamples;
		
	// We're done with all critical stuff. IRQs may be enabled again...
	OSRestoreInterrupts(old);
		
	return TRUE;
	}


/*!
 ******************************************************************************
 * \brief
 *		Receive data from bitstream and direct to required encoding facility
 *
 *		This function will receive the AUDD chunck from the VIDSimplePlayer's
 *		decode function. 
 *
 * \param bitstream
 *		Pointer to the data of a AUDD chunk
 *
 * \param bitstreamLen
 *		Length of the data in the AUDD chunk pointed to by above pointer
 *
 * \return
 *		FALSE if data could not be interpreted properly
 *
 ******************************************************************************
 */
BOOL VIDSimpleAudioDecode(const u8* bitstream, u32 bitstreamLen)
	{
	// Any audio data present?
	if (player.audioInfo.audioID != VID_FCC('N','O','N','E'))
		{
        // Select requested audio decoding method
        if(player.audioInfo.audioID == VID_FCC('V','A','U','D'))
			{
			u32 headerSize = ((VidAUDDVAUD*)bitstream)->size;
			// a channel mask if 0x3 selects the first two channels...
			if(!VIDAudioDecode(player.decoder, bitstream+headerSize, bitstreamLen-headerSize, 0x3, audioDecode, NULL))
				return FALSE;
			
			}
		else
			{
			// Calc data and call audio decode callback by ourself
			const u8* data = bitstream + sizeof(u32);
			u32 dataLength = *(const u32 *)bitstream;
			
			// We always assume 1 source data pointer
			if(!audioDecode(1, (const s16**)&data, VIDSimpleAudioSamplesFromBytes(dataLength), NULL))
				return FALSE;

			}
		audioPlayBackPossible = TRUE;
		}
	return TRUE;
	}

/*!
 ******************************************************************************
 * \brief
 *		Enable AI playback (within about 5ms)
 *
 ******************************************************************************
 */
				
void VIDSimpleAudioStartPlayback(const u32 *playMaskArray,u32 numMasks)
	{
	BOOL old = OSDisableInterrupts();
	
	if (audioPlayBackPossible)
		{
		audioPlayBufferEnabled = TRUE;
		
		VIDSimpleAudioChangePlayback(playMaskArray,numMasks);
		}
	
	OSRestoreInterrupts(old);
	}
	
/*!
 ******************************************************************************
 * \brief
 *		Change the active audio channels
 *
 ******************************************************************************
 */
				
void VIDSimpleAudioChangePlayback(const u32 *playMaskArray,u32 numMasks)
	{
	u32		i,b;
	BOOL	old = OSDisableInterrupts();
	
	audioPlayMaskArray = playMaskArray;
	audioNumPlayMasks = numMasks;
	
	// Any playback mask specified?
	if (audioNumPlayMasks != 0)
		{
		// Yes. Count the active voices...
		audioNumActiveVoices = 0;
		
		for(i=0; i<numMasks; i++)
			{
			for(b=0; b<32; b++)
				{
				if (audioPlayMaskArray[i] & (1<<b))
					audioNumActiveVoices++;
				}
			}
		
		// So, did we have too much?
		ASSERT(audioNumActiveVoices <= 2);
		}
	else
		audioNumActiveVoices = audioReadBufferNumChannels;		// Make all active...
	
	OSRestoreInterrupts(old);
	}

/*!
 ******************************************************************************
 * \brief
 *		Copy PCM16 data from the read buffer into the audio buffer
 *
 *		This function serves as an internal service function to make sure
 *		mono and stereo audio data gets properly converted into audio buffer
 *		format.
 *
 * \param smpOffset
 *		Offset in samples into the current write buffer
 *
 * \param numSamples
 *		Number of samples to be updated / copied
 *
 ******************************************************************************
 */
static void audioCopy(u32 smpOffset,u32 numSamples)
	{
	s16		*destAddrL,*destAddrR,*srcAddr,s;
	u32		j;
	
	// Target address in audio buffer
	destAddrL = (s16 *)((u32)audioPlayBuffer[audioPlayBufferWriteIndex] + smpOffset * sizeof(s16));
	destAddrR = (s16 *)((u32)audioPlayBuffer[audioPlayBufferWriteIndex] + smpOffset * sizeof(s16) + (VID_AUDIO_AIBUFFERSAMPLES * sizeof(s16)));

	// Single or dual channel setup?
    if (audioReadBufferNumChannels == 2)
		{
		// Stereo! Get source address of data...
		srcAddr = (s16 *)((u32)audioReadBuffer + audioReadBufferReadPos * 2 * sizeof(s16));
		
		// Copy samples into AI buffer and swap channels
		for(j=0; j<numSamples; j++)
			{
			*(destAddrL++) = *(srcAddr++);
			*(destAddrR++) = *(srcAddr++);
			}
		}
	else
		{
		// Mono case!
		
		// Make sure it's truely mono!
		ASSERT(audioReadBufferNumChannels == 1);
		
		// Get source address...
		srcAddr = (s16 *)((u32)audioReadBuffer + audioReadBufferReadPos * sizeof(s16));
		
		// Copy samples into AI buffer (AI is always stereo, so we have to dublicate data)
		for(j=0; j<numSamples; j++)
			{
			s = (*srcAddr++);
			*(destAddrL++) = s;
			*(destAddrR++) = s;
			}
		}
	
	// Advance read position as needed...
	audioReadBufferReadPos += numSamples;
	if (audioReadBufferReadPos >= audioReadBufferNumSamples)
		audioReadBufferReadPos -= audioReadBufferNumSamples;
	}

/*!
 ******************************************************************************
 * \brief
 *		AX callback
 *
 ******************************************************************************
 */
static void AXCallback(void)
	{
	u32		availSamples,availFrames,numUpdate,i,n;
	u32		audioPlayBufferNeededAudioFrames;
	u32		currentAddr;
	u32		leftSource, rightSource, leftTarget, rightTarget;

	// First thing to do here is call the regular soundfx callback.
	Sfx::AXUserCBack();
	
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	if (axPhase == AX_PHASE_START)
	{
		AXSetVoiceState(axVoice[0],AX_PB_STATE_RUN);
		AXSetVoiceState(axVoice[1],AX_PB_STATE_RUN);
		axPhase = AX_PHASE_PLAY;
	}
	else if( axPhase == AX_PHASE_PLAY )
	{
		if( axVoice[0]->pb.state == AX_PB_STATE_STOP )
		{
			return;
		}
	}
		
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	currentAddr = *(u32 *)&axVoice[0]->pb.addr.currentAddressHi;

	if (currentAddr >= axLastAddr)
		axPlayedSamples += currentAddr - axLastAddr;
	else
		axPlayedSamples += ((AX_ARAM_LEFT_CHANNEL + AX_ARAM_BUFFER_SIZE) - axLastAddr) + (currentAddr - AX_ARAM_LEFT_CHANNEL);
		
	axLastAddr = currentAddr;

	//OSReport("%d\n",axPlayedSamples);
		
	if (axPlayedSamples >= VID_AUDIO_AIBUFFERSAMPLES)
	{
		audioPlayBufferNeededAudioFrames = axPlayedSamples / VID_AUDIO_AIBUFFERSAMPLES;

		// Make sure that we never get an underrun we don't notice...
		if(!(audioPlayBufferNeededAudioFrames <= VID_AUDIO_NUMAIBUFFERS))
		{
//			OSReport("AX audio buffer underrun!\n");
			audioPlayBufferEnabled = false;
		}
		
		//ASSERT(audioPlayBufferNeededAudioFrames <= VID_AUDIO_NUMAIBUFFERS);

		// Is actual audio playback enabled?
		if (audioPlayBufferEnabled)
		{
			// How many samples could we get from the read buffer?
			if (audioReadBufferWritePos >= audioReadBufferReadPos)
				availSamples = audioReadBufferWritePos - audioReadBufferReadPos;
			else
				availSamples = audioReadBufferNumSamples - (audioReadBufferReadPos - audioReadBufferWritePos);
			
			// That's how many audio frames?
			availFrames = availSamples / VID_AUDIO_AIBUFFERSAMPLES;
    
			// Are the voice already playing?
			if (axPhase == AX_PHASE_PLAY)
			{
				f32 targetFrq;
				
				// Yes. We better watch out for the buffer status, so we don't get under-runs...
				if (availFrames < axMinAvailFrames)
					targetFrq = audioPlayBufferFrq * (1.0f - (1.0f/200.0f) * (axMinAvailFrames - availFrames));
				else
					targetFrq = (f32)audioPlayBufferFrq;
				
				// Track the target frequency quite slowly to avoid audible artifacts
                if (axCurrentFrq < targetFrq)
				{
					axCurrentFrq += audioPlayBufferFrq * 0.00125f;
					if (axCurrentFrq > targetFrq)
						axCurrentFrq = targetFrq;
				}
				else
				{
					axCurrentFrq -= audioPlayBufferFrq * 0.00125f;
					if (axCurrentFrq < targetFrq)
						axCurrentFrq = targetFrq;
				}
				
				//OSReport("%d %f (%d)\n",availFrames,axCurrentFrq,axMinAvailFrames);
					
				// Set frequency
				AXSetVoiceSrcRatio(axVoice[0],axCurrentFrq / (f32)AX_IN_SAMPLES_PER_SEC);
				AXSetVoiceSrcRatio(axVoice[1],axCurrentFrq / (f32)AX_IN_SAMPLES_PER_SEC);
			}

			//OSReport("AX: %d %d (%d)\n",availSamples,audioPlayBufferNeededAudioFrames * VID_AUDIO_AIBUFFERSAMPLES,axPlayedSamples);
	
			// So, how many can we update?
			numUpdate = (availFrames > audioPlayBufferNeededAudioFrames) ? audioPlayBufferNeededAudioFrames : availFrames;
	
			// If anything... go do it!
			if (numUpdate != 0)
			{
				axPlayedSamples -= numUpdate * VID_AUDIO_AIBUFFERSAMPLES;
				
				// Perform updates on each AI buffer in need of data...
				for(i=0; i<numUpdate; i++)
				{
					// Can we copy everything from a single source or does the data wrap around?
					if ((n = audioReadBufferNumSamples - audioReadBufferReadPos) < VID_AUDIO_AIBUFFERSAMPLES)
					{
						// It wraps...
						audioCopy(0,n);
						audioCopy(n,VID_AUDIO_AIBUFFERSAMPLES - n);
					}
					else
					{
						// We got one continous source buffer
						audioCopy(0,VID_AUDIO_AIBUFFERSAMPLES);
					}
						
					// Make sure the data ends up in real physical memory
					DCFlushRange(audioPlayBuffer[audioPlayBufferWriteIndex],VID_AUDIO_AIBUFFERSAMPLES * sizeof(s16) * 2);
					
					leftSource = (u32)audioPlayBuffer[audioPlayBufferWriteIndex];
					rightSource = (u32)audioPlayBuffer[audioPlayBufferWriteIndex] + (VID_AUDIO_AIBUFFERSAMPLES * sizeof(s16));
					leftTarget = 2 * (AX_ARAM_LEFT_CHANNEL + audioPlayBufferWriteIndex * VID_AUDIO_AIBUFFERSAMPLES);
					rightTarget = 2 * (AX_ARAM_RIGHT_CHANNEL + audioPlayBufferWriteIndex * VID_AUDIO_AIBUFFERSAMPLES);
					
					// Make sure we get this into ARAM ASAP...
					ARQPostRequest(&arqRequest[0][i%VID_AUDIO_NUMAIREQUESTS],0,ARQ_TYPE_MRAM_TO_ARAM,ARQ_PRIORITY_HIGH,leftSource,leftTarget,VID_AUDIO_AIBUFFERSAMPLES * sizeof(s16),NULL);
					ARQPostRequest(&arqRequest[1][i%VID_AUDIO_NUMAIREQUESTS],1,ARQ_TYPE_MRAM_TO_ARAM,ARQ_PRIORITY_HIGH,rightSource,rightTarget,VID_AUDIO_AIBUFFERSAMPLES * sizeof(s16),NULL);
					
					// Advance write index...
					audioPlayBufferWriteIndex = (u8)((audioPlayBufferWriteIndex + 1) % VID_AUDIO_NUMAIBUFFERS);
				}
				
				if (axPhase == AX_PHASE_STARTUP)
					axPhase = AX_PHASE_START;
			}
		}
		else
		{
			// Update buffer(s) with silence...
			axPlayedSamples -= audioPlayBufferNeededAudioFrames * VID_AUDIO_AIBUFFERSAMPLES;
			
			for(i=0; i<audioPlayBufferNeededAudioFrames; i++)
			{
				memset(audioPlayBuffer[audioPlayBufferWriteIndex],0,2 * sizeof(s16) * VID_AUDIO_AIBUFFERSAMPLES);
				DCFlushRange(audioPlayBuffer[audioPlayBufferWriteIndex],2 * sizeof(s16) * VID_AUDIO_AIBUFFERSAMPLES);
	
				leftSource = (u32)audioPlayBuffer[audioPlayBufferWriteIndex];
				rightSource = (u32)audioPlayBuffer[audioPlayBufferWriteIndex] + (VID_AUDIO_AIBUFFERSAMPLES * sizeof(s16)) / 2;
				leftTarget = 2 * (AX_ARAM_LEFT_CHANNEL + audioPlayBufferWriteIndex * VID_AUDIO_AIBUFFERSAMPLES);
				rightTarget = 2 * (AX_ARAM_RIGHT_CHANNEL + audioPlayBufferWriteIndex * VID_AUDIO_AIBUFFERSAMPLES);
				
				// Make sure we get this into ARAM ASAP...
				ARQPostRequest(&arqRequest[0][i%VID_AUDIO_NUMAIREQUESTS],0,ARQ_TYPE_MRAM_TO_ARAM,ARQ_PRIORITY_HIGH,leftSource,leftTarget,VID_AUDIO_AIBUFFERSAMPLES * sizeof(s16),NULL);
				ARQPostRequest(&arqRequest[1][i%VID_AUDIO_NUMAIREQUESTS],1,ARQ_TYPE_MRAM_TO_ARAM,ARQ_PRIORITY_HIGH,rightSource,rightTarget,VID_AUDIO_AIBUFFERSAMPLES * sizeof(s16),NULL);
				
				audioPlayBufferWriteIndex = (u8)((audioPlayBufferWriteIndex + 1) % VID_AUDIO_NUMAIBUFFERS);
			}
			
			if (axPhase == AX_PHASE_STARTUP)
				axPhase = AX_PHASE_START;
		}
	}
}
	
//############################################################################
//##                                                                        ##
//## Main entry point to the this example application.                      ##
//##                                                                        ##
//############################################################################

#endif		// DVDETH

namespace Flx
{

void PMovies_PlayMovie( const char *pName )
{
	NsDisplay::setBackgroundColor( (GXColor){0,0,0,0} );
	switch (VIGetTvFormat())
	{
		case VI_PAL:
		case VI_DEBUG_PAL:
			if ( !NxNgc::EngineGlobals.use_60hz )
			{
				rmode->viYOrigin = 0;
			}
			break;
		default:
			break;
	}
	VISetBlack( TRUE );
    VIConfigure(rmode);
    VIFlush();
    VIWaitForRetrace();

	g_legal = false;

	unsigned short last = ( padData[0].button | padData[1].button );

	Pcm::StopMusic();

#	ifndef DVDETH
//	GXRenderModeObj *rmode;	//, demoMode;
	u32		videoWidth, videoHeight;
	s32		surfaceX, surfaceY;
	s32		surfaceWidth, surfaceHeight;
//	u16 	buttonsDown;
	f64		totalTime = 0.0;
	f64		idealTime = 0.0;
	f32		idealTimeInc;
	BOOL	first = TRUE;
	BOOL 	xfbMode = FALSE;

#	ifdef USE_DIRECT_XFB
	xfbMode = TRUE;
#	endif

	OSInitFastCast();



	// Reset a bunch of GX state.

	// Color definitions
	
	#define GX_DEFAULT_BG (GXColor){64, 64, 64, 255}
	#define BLACK (GXColor){0, 0, 0, 0}
	#define WHITE (GXColor){255, 255, 255, 255}
	
	//
	// Render Mode
	//
	// (set 'rmode' based upon VIGetTvFormat(); code not shown)
	
	//
	// Geometry and Vertex
	//
	GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX2, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD3, GX_TG_MTX2x4, GX_TG_TEX3, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD4, GX_TG_MTX2x4, GX_TG_TEX4, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD5, GX_TG_MTX2x4, GX_TG_TEX5, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD6, GX_TG_MTX2x4, GX_TG_TEX6, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD7, GX_TG_MTX2x4, GX_TG_TEX7, GX_IDENTITY);
	GXSetNumTexGens(1);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetCullMode(GX_CULL_BACK);
	GXSetClipMode(GX_CLIP_ENABLE);
	GXSetNumChans(0); // no colors by default
	
	GXSetChanCtrl(
	GX_COLOR0A0,
	GX_DISABLE,
	GX_SRC_REG,
	GX_SRC_VTX,
	GX_LIGHT_NULL,
	GX_DF_NONE,
	GX_AF_NONE );
	
	GXSetChanAmbColor(GX_COLOR0A0, BLACK);
	GXSetChanMatColor(GX_COLOR0A0, WHITE);
	
	GXSetChanCtrl(
	GX_COLOR1A1,
	GX_DISABLE,
	GX_SRC_REG,
	GX_SRC_VTX,
	GX_LIGHT_NULL,
	GX_DF_NONE,
	GX_AF_NONE );
	
	GXSetChanAmbColor(GX_COLOR1A1, BLACK);
	GXSetChanMatColor(GX_COLOR1A1, WHITE);
	
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD2, GX_TEXMAP2, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD3, GX_TEXMAP3, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD4, GX_TEXMAP4, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD5, GX_TEXMAP5, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD6, GX_TEXMAP6, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD7, GX_TEXMAP7, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE8, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE9, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE10,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE11,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE12,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE13,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE14,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE15,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetNumTevStages(1);
	GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
	for ( int i = GX_TEVSTAGE0; i < GX_MAX_TEVSTAGE; i++) {
	GXSetTevKColorSel((GXTevStageID) i, GX_TEV_KCSEL_1_4 );
	GXSetTevKAlphaSel((GXTevStageID) i, GX_TEV_KASEL_1 );
	GXSetTevSwapMode ((GXTevStageID) i, GX_TEV_SWAP0, GX_TEV_SWAP0 );
	}





//	demoMode = GXNtsc480IntDf;
//	demoMode.fbWidth   = 640; //512;
//	
//	_DEMOInit(&demoMode, xfbMode);
//	
//	// Init AI interface in case we got audio data
//	if ( !AICheckInit() ) AIInit(NULL);
//	if ( !ARCheckInit() ) ARInit(NULL,0);
//	if ( !ARQCheckInit() ) ARQInit();
//	AXInit();

	// If we want to use the 'Locked Cache', we need to enable it here!
	// The usage of 'Locked Cache' is optional, but speeds up decoding time a little bit
	LCEnable();
	lcMemBase = (u8*)LCGetBase();
	
	// In xfb mode, we need some memory for the conversion stuff.
	// We offset our locked cache 'base addr' by the required number of bytes!
	// NOTE: This mem is only used TEMPORARY during the VIDXFBDraw() function!
	if(xfbMode)
		{
		xfbLCStart = lcMemBase;
		lcMemBase += VIDXFBGetLCSize();
		}

//	rmode = DEMOGetRenderModeObj();

	VIDSimpleInit(myAlloc, myFree, myAllocFromLC);

//	if (VIDSimpleOpen(VIDEO_FILENAME, FALSE) == FALSE)




	// Incoming movie name is in the form movies\<name>, convert to movies/vid\<name>.vid.
	char name_conversion_buffer[256] = "movies/vid/";
	int length		= strlen( pName );
	int backwards	= length;
	while( backwards )
	{
		if( pName[backwards] == '\\' )
		{
			++backwards;
			break;
		}
		--backwards;
	}
	strncpy( name_conversion_buffer + 11, pName + backwards, length - backwards );
	length = strlen( name_conversion_buffer );
	name_conversion_buffer[length] = '.';
	name_conversion_buffer[length + 1] = 'v';
	name_conversion_buffer[length + 2] = 'i';
	name_conversion_buffer[length + 3] = 'd';
	name_conversion_buffer[length + 4] = 0;







	if (VIDSimpleOpen(name_conversion_buffer, FALSE) == FALSE) goto quit;
//		OSHalt("*** VIDSimpleOpen failed!\n");

	if (VIDSimpleAllocDVDBuffers() == FALSE)
	{
		VIDSimpleLoadStop();
		VIDSimpleClose();

        goto quit;
	}
//		OSHalt("*** VIDSimpleAllocDVDBuffers failed!\n");
	
	if (VIDSimpleCreateDecoder(TRUE) == FALSE)
	{
		VIDSimpleLoadStop();
		VIDSimpleClose();
		VIDSimpleFreeDVDBuffers();

        goto quit;
	}
//		OSHalt("*** VIDSimpleCreateDecoder failed!\n");
		
	if (VIDSimpleInitAudioDecoder())
	{
		VIDSimpleLoadStop();
		VIDSimpleClose();
		VIDSimpleFreeDVDBuffers();
		VIDSimpleDestroyDecoder();

        goto quit;
	}
//		OSHalt("*** VIDSimpleInitAudioDecoder failed!\n");






	// Preload all DVD buffers and set 'loop' mode
//	VIDSimpleLoadStart(TRUE);
	VIDSimpleLoadStart(FALSE);

	VIDSimpleGetVideoSize(&videoWidth, &videoHeight);
	
	// Calculate width and height to be used to display movie
	surfaceWidth = rmode->fbWidth;
	surfaceHeight = (s32)(videoHeight * ((f32)rmode->fbWidth / (f32)videoWidth));
	
	// Get the pixels "square"
	surfaceHeight = (s32)(surfaceHeight * (640.0f / (f32)rmode->fbWidth));	// assuming 480 vertical at all times!
	
	// Calculate offset to put it into the center of the screen
	surfaceX = 0;
	surfaceY = ((s32)rmode->xfbHeight - (s32)surfaceHeight) / 2;
	if (surfaceY < 0)
		surfaceY = 0;
	
//#ifdef MY_DEBUG
//	OSReport("Resolution: %dx%d\n",videoWidth,videoHeight);
//	OSReport("Display resolution: %dx%d\n",surfaceWidth,surfaceHeight);
//	OSReport("Rate: %f fps\n",VIDSimpleGetFPS());
//	OSReport("Frames: %d\n",VIDSimpleGetFrameCount());
//	VIDSimpleAudioGetInfo(&header);
//	if(header.audioID != VID_FCC('N','O','N','E'))
//		{
//		OSReport("Audio codec: %4s\n", &header.audioID);
//		OSReport("Channels: %d\n", (u32)header.pcm16.numChannels);
//		OSReport("Sample rate: %d Hz\n", header.pcm16.frq);
//		}
//#endif

	idealTimeInc = 1000.0f / VIDSimpleGetFPS();

		while (1)
		{
		OSTick startTicks;
	
		startTicks = OSGetTick();

		VISetBlack( FALSE );
		NsDisplay::begin();
		NsRender::begin();
//		_DEMOBeforeRender();
		
		// Decode one frame
		if (VIDSimpleDecode() == FALSE)
		{
			// Decode failed, usually just due to the end of the movie being reached.
			break;
		}

		// We deomstrate two different drawing methods
		if(!xfbMode)
		{
			// Render decoded frame as texture. YUV conversion is handled by hardware.
			VIDSimpleDraw(rmode, (u32)surfaceX, (u32)surfaceY, (u32)surfaceWidth, (u32)surfaceHeight);
		}
		
		// Wait for frame buffer swap to be done (we might have triggered one in the last loop)
//		_DEMOWaitFrameBuffer();
		
		if (xfbMode)
			{
			// Render image into the XFB immediately
			VIDSimpleDrawXFB(rmode, xfbLCStart);
			}
			
		s32 error;
		error = DVDGetDriveStatus();
		if ( ( error != DVD_STATE_END ) &&
			 ( error != DVD_STATE_BUSY ) &&
			 ( error != DVD_STATE_CANCELED ) &&
			 ( error != DVD_STATE_PAUSING ) &&
			 ( error != DVD_STATE_WAITING ) )


			{
				switch (VIGetTvFormat())
				{
					case VI_PAL:
					case VI_DEBUG_PAL:
						if ( !NxNgc::EngineGlobals.use_60hz )
						{
							rmode->viYOrigin = 23;
						}
						break;
					default:
						break;
				}
				VIConfigure(rmode);

				// Stop audio, turn volume to 0.
				AXSetVoiceState(axVoice[0],AX_PB_STATE_STOP);
				AXSetVoiceState(axVoice[1],AX_PB_STATE_STOP);

				hwGXInit();
	
				GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
				GXSetColorUpdate(GX_ENABLE);
				GXSetAlphaUpdate(GX_ENABLE);

				NxNgc::EngineGlobals.screen_brightness = 1.0f;

				// Display n
				while ( 1 )
				{
					error = DVDGetDriveStatus();
//					printf( "Error code: %d\n", error );

					NsDisplay::doReset();

					if ( error == DVD_STATE_END ) break;
					if ( error == DVD_STATE_BUSY ) continue;
					if ( error == DVD_STATE_CANCELED ) break;
					if ( error == DVD_STATE_PAUSING ) continue;
					if ( error == DVD_STATE_WAITING ) continue;

					// Render the text.
					NsDisplay::begin();
					NsRender::begin();
	
					NsCamera cam;
					cam.orthographic( 0, 0, rmode->fbWidth, 448 );
	
					// Draw the screen.
					NsPrim::begin();
	
					cam.begin();
	
					GXSetZMode( GX_FALSE, GX_ALWAYS, GX_TRUE );
	
					NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_BLEND );
	
		//			if ( NsDisplay::shouldReset() )
		//			{
		//				// Reset message.
		//				Script::RunScript( "ngc_reset" );
		//			}
		//			else
					{
						// DVD Error message.
						switch ( error )
						{
							case DVD_STATE_FATAL_ERROR:
								Script::RunScript( "ngc_dvd_fatal" );
								NxNgc::EngineGlobals.disableReset = true;
								break;
							case DVD_STATE_RETRY:
								Script::RunScript( "ngc_dvd_retry" );
								break;
							case DVD_STATE_COVER_OPEN:
								Script::RunScript( "ngc_dvd_cover_open" );
								NxNgc::EngineGlobals.disableReset = false;
								break;
							case DVD_STATE_NO_DISK:
								Script::RunScript( "ngc_dvd_no_disk" );
								NxNgc::EngineGlobals.disableReset = false;
								break;
							case DVD_STATE_WRONG_DISK:
								Script::RunScript( "ngc_dvd_wrong_disk" );
								NxNgc::EngineGlobals.disableReset = false;
								break;
							default:
								break;
						}
					}
	
					NsDisplay::setBackgroundColor( messageColor );
	
					cam.end();
	
					NsPrim::end();
	
					NsRender::end();
					NsDisplay::end( true );
				}

				// Reset a bunch of GX state.
			
				NsDisplay::setBackgroundColor( (GXColor){0,0,0,0} );

				// Color definitions
				
				#define GX_DEFAULT_BG (GXColor){64, 64, 64, 255}
				#define BLACK (GXColor){0, 0, 0, 0}
				#define WHITE (GXColor){255, 255, 255, 255}
				
				//
				// Render Mode
				//
				// (set 'rmode' based upon VIGetTvFormat(); code not shown)
				
				//
				// Geometry and Vertex
				//
				GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
				GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY);
				GXSetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX2, GX_IDENTITY);
				GXSetTexCoordGen(GX_TEXCOORD3, GX_TG_MTX2x4, GX_TG_TEX3, GX_IDENTITY);
				GXSetTexCoordGen(GX_TEXCOORD4, GX_TG_MTX2x4, GX_TG_TEX4, GX_IDENTITY);
				GXSetTexCoordGen(GX_TEXCOORD5, GX_TG_MTX2x4, GX_TG_TEX5, GX_IDENTITY);
				GXSetTexCoordGen(GX_TEXCOORD6, GX_TG_MTX2x4, GX_TG_TEX6, GX_IDENTITY);
				GXSetTexCoordGen(GX_TEXCOORD7, GX_TG_MTX2x4, GX_TG_TEX7, GX_IDENTITY);
				GXSetNumTexGens(1);
				GXSetCurrentMtx(GX_PNMTX0);
				GXSetCullMode(GX_CULL_BACK);
				GXSetClipMode(GX_CLIP_ENABLE);
				GXSetNumChans(0); // no colors by default
				
				GXSetChanCtrl(
				GX_COLOR0A0,
				GX_DISABLE,
				GX_SRC_REG,
				GX_SRC_VTX,
				GX_LIGHT_NULL,
				GX_DF_NONE,
				GX_AF_NONE );
				
				GXSetChanAmbColor(GX_COLOR0A0, BLACK);
				GXSetChanMatColor(GX_COLOR0A0, WHITE);
				
				GXSetChanCtrl(
				GX_COLOR1A1,
				GX_DISABLE,
				GX_SRC_REG,
				GX_SRC_VTX,
				GX_LIGHT_NULL,
				GX_DF_NONE,
				GX_AF_NONE );
				
				GXSetChanAmbColor(GX_COLOR1A1, BLACK);
				GXSetChanMatColor(GX_COLOR1A1, WHITE);
				
				GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
				GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
				GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD2, GX_TEXMAP2, GX_COLOR0A0);
				GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD3, GX_TEXMAP3, GX_COLOR0A0);
				GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD4, GX_TEXMAP4, GX_COLOR0A0);
				GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD5, GX_TEXMAP5, GX_COLOR0A0);
				GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD6, GX_TEXMAP6, GX_COLOR0A0);
				GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD7, GX_TEXMAP7, GX_COLOR0A0);
				GXSetTevOrder(GX_TEVSTAGE8, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
				GXSetTevOrder(GX_TEVSTAGE9, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
				GXSetTevOrder(GX_TEVSTAGE10,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
				GXSetTevOrder(GX_TEVSTAGE11,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
				GXSetTevOrder(GX_TEVSTAGE12,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
				GXSetTevOrder(GX_TEVSTAGE13,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
				GXSetTevOrder(GX_TEVSTAGE14,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
				GXSetTevOrder(GX_TEVSTAGE15,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
				GXSetNumTevStages(1);
				GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
				GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
				for ( int i = GX_TEVSTAGE0; i < GX_MAX_TEVSTAGE; i++) {
				GXSetTevKColorSel((GXTevStageID) i, GX_TEV_KCSEL_1_4 );
				GXSetTevKAlphaSel((GXTevStageID) i, GX_TEV_KASEL_1 );
				GXSetTevSwapMode ((GXTevStageID) i, GX_TEV_SWAP0, GX_TEV_SWAP0 );

				// Restart audio, volume to full.
				AXSetVoiceState(axVoice[0],AX_PB_STATE_RUN);
				AXSetVoiceState(axVoice[1],AX_PB_STATE_RUN);

				}

				switch (VIGetTvFormat())
				{
					case VI_PAL:
					case VI_DEBUG_PAL:
						if ( !NxNgc::EngineGlobals.use_60hz )
						{
							rmode->viYOrigin = 0;
						}
						break;
					default:
						break;
				}
				VIConfigure(rmode);

//			OSHalt("*** DVD error occured. A true DVD error handler is not part of this demo.\n");
				startTicks = OSGetTick();
			}
			
		// Final processing for this frame (copy out etc.)
//		_DEMODoneRender();
		NsRender::end();
		NsDisplay::end();

//		// handle GC controller
//		DEMOPadRead();
//		buttonsDown = DEMOPadGetButtonDown(0);
//		if(buttonsDown & PAD_BUTTON_START)
//			break;
			
		unsigned short current = ( padData[0].button | padData[1].button );
		unsigned short press = ( current ^ last ) & current;
		last = current;

		if ( press & ( PAD_BUTTON_START | PAD_BUTTON_A ) ) break;

		// Control timing
		if (!first)
			{
			f64	dt;
			idealTime += 1000.0 / VIDSimpleGetFPS();
			do
				{
				dt = OSTicksToMilliseconds((f64)(OSGetTick() - startTicks));
				}
			while((totalTime + dt) < idealTime);
			totalTime += dt;
			}
		else
			first = FALSE;
			
		} // while (1)
		
	// free our ressources. Just as demonstration what you need to do for a 'clean' exit
	VIDSimpleLoadStop();
	VIDSimpleClose();

	VIDSimpleFreeDVDBuffers();
	VIDSimpleDestroyDecoder();
	VIDSimpleExitAudioDecoder();

quit:
	NsDisplay::setBackgroundColor( (GXColor){0,0,0,0} );

//	GXSetPixelFmt(/*GX_PF_RGB8_Z24*/GX_PF_RGBA6_Z24, GX_ZC_LINEAR);

	GQRSetup6( GQR_SCALE_64,		// Pos
			   GQR_TYPE_S16,
			   GQR_SCALE_64,
			   GQR_TYPE_S16 );
	GQRSetup7( 14,		// Normal
			   GQR_TYPE_S16,
			   14,
			   GQR_TYPE_S16 );

	hwGXInit();

	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetColorUpdate(GX_ENABLE);
	GXSetAlphaUpdate(GX_ENABLE);

	// Register our default AX callback.
	AXRegisterCallback( Sfx::AXUserCBack );

	LCDisable();

#	ifdef MY_DEBUG
	OSReport("VidPlayer done!\n");	
#	endif	

#	endif		// DVDETH

	switch (VIGetTvFormat())
	{
		case VI_PAL:
		case VI_DEBUG_PAL:
			if ( !NxNgc::EngineGlobals.use_60hz )
			{
				rmode->viYOrigin = 23;
			}
			break;
		default:
			break;
	}
	VISetBlack( TRUE );
    VIConfigure(rmode);
    VIFlush();
    VIWaitForRetrace();
	VISetBlack( FALSE );
}
	

bool Movie_Render ( void )
{
//	if ( !playing ) return false;
//
//	//
//	// Decompress the Bink frame.
//	//
//	
//	BinkDoFrame( Bink );
//	
//	//
//	// Draw the next frame.
//	//
//	
//	Show_frame( Bink );
//	
//	//
//	// Keep playing the movie.
//	//
//	
//	BinkNextFrame( Bink );
//
//	return true;

	return false;
}

} // namespace Flx




