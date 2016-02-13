#ifndef DVDETH
/*!
 ******************************************************************************
 * \file AUDSimplePlayer.cpp
 *
 * \brief
 *		This file provides the required player control functions.
 *
 * \note
 *      This is a demonstration source only!
 *
 * \date
 *		05/21/03
 *
 * \version
 *		1.0
 *
 * \author
 *		Achim Moller
 *
 ******************************************************************************
 */

/******************************************************************************
 *  INCLUDES
 ******************************************************************************/
#include <dolphin.h>
#include <string.h>

#include "AUDSimplePlayer.h"
#include "AUDSimpleAudio.h"

/******************************************************************************
 *  GLOABL VARIABLES
 ******************************************************************************/
AudSimplePlayer audio_player;

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
 *		Initializes all audio_player structures.
 *
 ******************************************************************************
 */
void AUDSimpleInit(VAUDAllocator _cbAlloc, VAUDDeallocator _cbFree)
	{
	memset(&audio_player, 0, sizeof(audio_player));
	audio_player.cbAlloc = _cbAlloc;
	audio_player.cbFree = _cbFree;
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
	if (!audio_player.error && audio_player.preFetchState == TRUE)
		{
		if (audio_player.currentFrameCount >  audio_player.audioInfo.vaudex.frameCount - 1)
			{
			if (audio_player.loopMode)
				{
				audio_player.currentFrameCount = 0;
				audio_player.nextFrameOffset = audio_player.firstFrameOffset;
				audio_player.nextFrameSize  = audio_player.firstFrameSize;
				}
			else
				return;
			}
		
		audio_player.asyncDvdRunning = TRUE;

		if (DVDReadAsync(&audio_player.fileHandle,
						 audio_player.readBuffer[audio_player.readIndex].ptr,
						 (s32)audio_player.nextFrameSize,
						 (s32)audio_player.nextFrameOffset, dvdDoneCallback) != TRUE )
			{
			audio_player.asyncDvdRunning = FALSE;
			audio_player.error = TRUE;
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
		audio_player.error = TRUE;
		return;
		}
	else if (result == DVD_RESULT_CANCELED)
		{
		return;
		}

	audio_player.asyncDvdRunning = FALSE;

	audio_player.readBuffer[audio_player.readIndex].frameNumber = audio_player.currentFrameCount;
	audio_player.readBuffer[audio_player.readIndex].size = (u32)result;	
	audio_player.readBuffer[audio_player.readIndex].valid = TRUE;
	
	audio_player.currentFrameCount++;

	// move file pointer
	audio_player.nextFrameOffset += audio_player.nextFrameSize;
	
	if(audio_player.currentFrameCount < audio_player.audioInfo.vaudex.frameCount)
		{
		// set read size for next 'FRAM' chunk
		u32* nextFrameStart = (u32*)(audio_player.readBuffer[audio_player.readIndex].ptr + audio_player.nextFrameSize - 32);
	
		// some check if file structure is okay
		ASSERT(nextFrameStart[0] == VID_FCC('F','R','A','M'));
		
		// get the size of the next 'FRAM' chunk to read
		audio_player.nextFrameSize = nextFrameStart[1];
		ASSERT(audio_player.nextFrameSize);
		}
	else
		audio_player.nextFrameSize = 0;	// at end of file we have a size of '0'. This should be reinitialized later
									// using the size of the first frame somwhere else! Otherwise, we get an assertion

	// use next buffer
	audio_player.readIndex = (audio_player.readIndex + 1) % AUD_NUM_READ_BUFFERS;

	// continue loading if we have a free buffer
	if (!audio_player.readBuffer[audio_player.readIndex].valid)
		ReadFrameAsync();
	}

/*!
 ******************************************************************************
 * \brief
 *		Allocate buffer memory for asynchronous dvd read
 *
* \return
 *		TRUE if DVD buffer setup was successful.
 *
 ******************************************************************************
 */
BOOL AUDSimpleAllocDVDBuffers(void)
	{
	u32 i;
	u32 bufferSize;
	u8* ptr;

	bufferSize = audio_player.audioInfo.vaudex.maxBufferSize;
	ASSERT(bufferSize);
	
	bufferSize += VID_CHUNK_HEADER_SIZE;	// 'fram' header
	bufferSize += VID_CHUNK_HEADER_SIZE;	// 'vidd' header
	bufferSize = OSRoundUp32B(bufferSize);
	
	ASSERT(audio_player.cbAlloc);
	audio_player.readBufferBaseMem = (u8*)((*audio_player.cbAlloc)(bufferSize * AUD_NUM_READ_BUFFERS));
	
	if(!audio_player.readBufferBaseMem)
		return FALSE;	// out of mem
	
	ptr = audio_player.readBufferBaseMem;
	for (i = 0; i < AUD_NUM_READ_BUFFERS ; i++)
		{
		audio_player.readBuffer[i].ptr = ptr;
		ptr += bufferSize;
		audio_player.readBuffer[i].valid = FALSE;
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
void AUDSimpleFreeDVDBuffers(void)
	{
	ASSERT(audio_player.cbFree);
	ASSERT(audio_player.readBufferBaseMem);
	(*audio_player.cbFree)(audio_player.readBufferBaseMem);
	}

/*!
 ******************************************************************************
 * \brief
 *		Create a new decoder instance.
 *
 * \return
 *		TRUE if decoder creation was successful
 *
 ******************************************************************************
 */
BOOL AUDSimpleCreateDecoder(void)
	{
	u32 skip;
	
	audio_player.decoder = VAUDCreateDecoder(audio_player.cbAlloc, audio_player.cbFree, audio_player.audioInfo.vaud.maxHeap);
	
	if(!audio_player.decoder)
		return FALSE;

	ASSERT(audio_player.audioHeaderChunk);
	ASSERT(audio_player.audioInfo.vaud.maxHeap > 0);
	ASSERT(audio_player.audioInfo.vaud.preAlloc > 0);
	skip = VID_CHUNK_HEADER_SIZE + sizeof(u32) + (audio_player.audioInfo.vaudex.version > 0 ? audio_player.audioInfo.vaudex.size : sizeof(VidAUDHVAUD));

	if(!VAUDInitDecoder(audio_player.decoder, audio_player.audioHeaderChunk + skip, ((VidChunk*)audio_player.audioHeaderChunk)->len - skip, audio_player.audioInfo.vaud.preAlloc))
		{
		VAUDDestroyDecoder(audio_player.decoder);
		return FALSE;
		}
	
	return TRUE;
	}
/*!
 ******************************************************************************
 * \brief
 *		Destroy decoder instance.
 *
 *		At this point the decoder returns all allocated memory by using
 *		the cbFree callback.
 *
 ******************************************************************************
 */
void AUDSimpleDestroyDecoder(void)
{
	ASSERT(audio_player.decoder);
	VAUDDestroyDecoder(audio_player.decoder);
	
	// Set the decoder to NULL.
	audio_player.decoder = NULL;
}



static u32 readNum = 0;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void AUDSimpleLoadStartDVDCallback( s32 result, DVDFileInfo* fileInfo )
{
	// Deal with errors.
	if( audio_player.error )
	{
		return;
	}
	
	audio_player.nextFrameOffset += audio_player.nextFrameSize;
	audio_player.readBuffer[audio_player.readIndex].size = audio_player.nextFrameSize;

	// Set read size for next 'FRAM' chunk.
	u32* nextFrame = (u32*)(audio_player.readBuffer[audio_player.readIndex].ptr + audio_player.nextFrameSize - 32);

	// Some sanity check if file structure is valid!
	ASSERT( nextFrame[0] == VID_FCC( 'F','R','A','M' ));

	audio_player.nextFrameSize = nextFrame[1];
	ASSERT( audio_player.nextFrameSize );

	audio_player.readBuffer[audio_player.readIndex].valid = TRUE;
	audio_player.readBuffer[audio_player.readIndex].frameNumber = audio_player.currentFrameCount;

	// Use next buffer.
	audio_player.readIndex = (audio_player.readIndex + 1) % AUD_NUM_READ_BUFFERS;
	audio_player.currentFrameCount++;
	
	if( --readNum > 0 )
	{
		if( DVDReadAsync( &audio_player.fileHandle, audio_player.readBuffer[audio_player.readIndex].ptr, (s32)audio_player.nextFrameSize, (s32)audio_player.nextFrameOffset, AUDSimpleLoadStartDVDCallback ) < 0 )
		{
#			ifdef _DEBUG
			OSReport("*** AUDSimpleLoadStart: Failed to read from file.\n");
#			endif
			audio_player.error = TRUE;
			return;
		}
	}
	else
	{
		// All done.
		audio_player.loopMode		= FALSE;
		audio_player.preFetchState	= TRUE;
	}
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
BOOL AUDSimpleLoadStart(BOOL loopMode)
{
//	u32		i, readNum;
//	u32		i;
//	u32*	nextFrame;

	ASSERT( loopMode == FALSE );
	
	if( audio_player.open && audio_player.preFetchState == FALSE )
	{
		readNum = AUD_NUM_READ_BUFFERS;

		// In 'non-loop' mode we must take care if we have LESS frames than preloading buffers
        if( !loopMode && audio_player.audioInfo.vaudex.frameCount < AUD_NUM_READ_BUFFERS )
			readNum = audio_player.audioInfo.vaudex.frameCount;
				
//		for( i = 0; i < readNum; i++ )
		if( readNum > 0 )
		{
			// Read total 'FRAM' chunk and 32 bytes of NEXT chunk.
//			if( DVDRead( &audio_player.fileHandle, ptr, (s32)audio_player.nextFrameSize, (s32)audio_player.nextFrameOffset ) < 0 )
			if( DVDReadAsync( &audio_player.fileHandle, audio_player.readBuffer[audio_player.readIndex].ptr, (s32)audio_player.nextFrameSize, (s32)audio_player.nextFrameOffset, AUDSimpleLoadStartDVDCallback ) < 0 )
			{
#				ifdef _DEBUG
				OSReport("*** AUDSimpleLoadStart: Failed to read from file.\n");
#				endif
				audio_player.error = TRUE;
				return FALSE;
			}
			
//			audio_player.nextFrameOffset += audio_player.nextFrameSize;
//			audio_player.readBuffer[audio_player.readIndex].size = audio_player.nextFrameSize;

            // set read size for next 'FRAM' chunk
//			nextFrame = (u32*)(audio_player.readBuffer[audio_player.readIndex].ptr + audio_player.nextFrameSize - 32);
			
			// some sanity check if file structure is valid!
//			ASSERT(nextFrame[0] == VID_FCC('F','R','A','M'));
			
//			audio_player.nextFrameSize = nextFrame[1];
//			ASSERT( audio_player.nextFrameSize );

//			audio_player.readBuffer[audio_player.readIndex].valid = TRUE;
//			audio_player.readBuffer[audio_player.readIndex].frameNumber = audio_player.currentFrameCount;

			// Use next buffer
//			audio_player.readIndex = (audio_player.readIndex + 1) % AUD_NUM_READ_BUFFERS;

//			audio_player.currentFrameCount++;

//			if (audio_player.currentFrameCount >  audio_player.audioInfo.vaudex.frameCount - 1)
//			{
//				if (loopMode)
//				{
//					audio_player.currentFrameCount = 0;
//					audio_player.nextFrameOffset = audio_player.firstFrameOffset;
//					audio_player.nextFrameSize  = audio_player.firstFrameSize;
//				}
//			}
		}
//		audio_player.loopMode = loopMode;
//		audio_player.preFetchState = TRUE;
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
 *		TRUE if audio_player could be stopped!		
 *
 ******************************************************************************
 */
BOOL AUDSimpleLoadStop(void)
	{
	u32 i;

	if (audio_player.open)
		{
		// stop preloading process
		audio_player.preFetchState = FALSE;

		if (audio_player.asyncDvdRunning)
			{
			DVDCancel(&audio_player.fileHandle.cb);
			audio_player.asyncDvdRunning = FALSE;
			}

		// invalidate all buffers
		for (i = 0 ; i < AUD_NUM_READ_BUFFERS; i++)
			audio_player.readBuffer[i].valid = FALSE;

		audio_player.nextFrameOffset = audio_player.firstFrameOffset;
		audio_player.nextFrameSize = audio_player.firstFrameSize;
		audio_player.currentFrameCount = 0;
		
		audio_player.error 	   		= FALSE;
		audio_player.readIndex   		= 0;
		audio_player.decodeIndex 		= 0;

		return TRUE;
		}
	return FALSE;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void AUDSimpleOpenDVDCallback( s32 result, DVDFileInfo* fileInfo )
{
	static u32	fileOffset		= 0;
	static u32	headSize		= 0;
	static u32	audioInfoSize	= 0;
	
	// Deal with errors, possibly flagged as an indication to shut down immediately.
	if( audio_player.error )
	{
		// Set this back to zero so the player will be regarded as 'free' again.
		audio_player.asyncOpenCallbackStatus = 0;
		return;
	}
	
	switch( audio_player.asyncOpenCallbackStatus )
	{
		case 1:
		{
			// The read of the 'VID1' chunk has completed.
			fileOffset = 32;
	
			// Check file id.
			if( workChunk.id != VID_FCC('V','I','D','1' ))
			{
#				ifdef _DEBUG
				OSReport("*** No VID1 file: '%s'\n", fileName);
#				endif
				DVDClose( &audio_player.fileHandle );
				audio_player.asyncOpenCallbackStatus = ASYNC_OPEN_FAIL;
				return;
			}
	
			// Check for correct version of vid chunk.
			// If we find this version we assume a 'special' alignment and chunk ordering which may be invalid
			// in another version of the file format.
			if( workChunk.vid.versionMajor != 1 || workChunk.vid.versionMinor != 0 )
			{
#				ifdef _DEBUG
				OSReport("*** Unsupported file version: major: %d, minor: %d\n", workChunk.vid.versionMajor, workChunk.vid.versionMajor);
#				endif
				DVDClose( &audio_player.fileHandle );
				audio_player.asyncOpenCallbackStatus = ASYNC_OPEN_FAIL;
				return;
			}
	
#			ifdef _DEBUG
			// Sometimes, it's required to check for a special build of the VidConv converter.
			{
				u32 version = VID_VERSION(workChunk.vid.vidConvMajor, workChunk.vid.vidConvMinor, workChunk.vid.vidConvBuild);
				if( version < VID_VERSION( 1, 0, 1 ))
					OSReport("*** WARNING: Vid file created using an unsupported converter version: %d.%d.%d\n", (u32)workChunk.vid.vidConvMajor, (u32)workChunk.vid.vidConvMinor, (u32)workChunk.vid.vidConvBuild);
			}
#			endif

			// Set callback status to indicate 'VID1' chunk is read.
			audio_player.asyncOpenCallbackStatus = 2;
			
			// Check types of chunks we have in this file.
			// !!! Note that we assume start of 'HEAD' chunk at byte offset 32 from file start !!!
			if( DVDReadAsync( &audio_player.fileHandle, &workChunk, 32, (s32)fileOffset, AUDSimpleOpenDVDCallback ) < 0 )
			{
#				ifdef _DEBUG
				OSReport("*** Failed to read 'HEAD' chunk.\n");
#				endif
				DVDClose( &audio_player.fileHandle );
				audio_player.asyncOpenCallbackStatus = ASYNC_OPEN_FAIL;
				return;
			}
			break;
		}
	
		case 2:
		{
			// The read of the chunk type info has completed.
			if( workChunk.id != VID_FCC('H','E','A','D' ))
			{
#				ifdef _DEBUG
				OSReport("*** No HEAD chunk found at expected offset\n");
#				endif
				DVDClose( &audio_player.fileHandle );
				audio_player.asyncOpenCallbackStatus = ASYNC_OPEN_FAIL;
				return;
			}

			// Calculate the start of the first frame chunk (we know the header chunk starts at offset 32).
			audio_player.nextFrameOffset = workChunk.len + 32;

			// Skip 'HEAD' chunk id, len and version fields.
			fileOffset += VID_CHUNK_HEADER_SIZE;

			// The header chunk contains one or more header chunks for the different data types contained
			// in the stream. Parse them all...

			headSize = workChunk.len - VID_CHUNK_HEADER_SIZE;
			
			audio_player.asyncOpenCallbackStatus = 3;
			if( DVDReadAsync( &audio_player.fileHandle, &workChunk, 32, (s32)fileOffset, AUDSimpleOpenDVDCallback ) < 0 )
			{
#				ifdef _DEBUG
				OSReport("*** Error reading file at offset %d\n", fileOffset);
#				endif
				DVDClose( &audio_player.fileHandle );
				audio_player.asyncOpenCallbackStatus = ASYNC_OPEN_FAIL;
				return;
			}
			break;
		}
	
		case 3:
		{
			fileOffset	+= 32;
			headSize	-= 32;

			// We analyze the 1st 32 bytes of the chunk for a known header format.
			if( workChunk.id == VID_FCC( 'A','U','D','H' ))
			{
				// Allocate memory for audio header chunk.
				audio_player.audioHeaderChunk	= (u8*)((*audio_player.cbAlloc)(workChunk.len));
				audioInfoSize					= workChunk.len - VID_CHUNK_HEADER_SIZE;

				// Copy the already loaded part.
				memcpy( audio_player.audioHeaderChunk, &workChunk, 32 );
				workChunk.len -= 32;

				// Read additional audio header bytes if the audio header is greater that 32 bytes
				if( workChunk.len >= 32 )
				{
					ASSERT(( workChunk.len & 31 ) == 0 );
					audio_player.asyncOpenCallbackStatus = 4;
					if( DVDReadAsync( &audio_player.fileHandle, audio_player.audioHeaderChunk + 32, workChunk.len, (s32)fileOffset, AUDSimpleOpenDVDCallback ) < 0 )
					{
#						ifdef _DEBUG
						OSReport("*** Error reading file at offset %d\n", fileOffset);
#						endif
						DVDClose( &audio_player.fileHandle );
						audio_player.asyncOpenCallbackStatus = ASYNC_OPEN_FAIL;
						return;
					}
				}
				else
				{
					ASSERT( 0 );
				}
			}
			else
			{
				ASSERT( 0 );
			}
			break;
		}
					
		case 4:
		{
			fileOffset	+= workChunk.len;
			headSize	-= workChunk.len;

			// Setup and calc the number of bytes which we are allowed to copy into the audioInfo struct
			memcpy( &audio_player.audioInfo, audio_player.audioHeaderChunk + VID_CHUNK_HEADER_SIZE, MIN( audioInfoSize, sizeof( audio_player.audioInfo )));
			
			// Check if we have the correct vaud file version (>0).
			if( audio_player.audioInfo.vaudex.version == 0 )
			{
#				ifdef _DEBUG
				OSReport("*** Invalid version in vaud file.");
#				endif
				DVDClose( &audio_player.fileHandle );
				audio_player.asyncOpenCallbackStatus = ASYNC_OPEN_FAIL;
				return;
			}

			// We can only play audio files which have the following fields set.
			// Note that in case of VIDEO files this fields are allowed to be 0.
			ASSERT( audio_player.audioInfo.vaudex.maxBufferSize > 0 );
			ASSERT( audio_player.audioInfo.vaudex.frameCount > 0 );
			ASSERT( audio_player.audioInfo.vaudex.frameTimeMs > 0 );

			// Read beginning of 1st frame chunk to get required size information.
			audio_player.asyncOpenCallbackStatus = 5;
			if( DVDReadAsync( &audio_player.fileHandle, &workChunk, 32 , (s32)audio_player.nextFrameOffset, AUDSimpleOpenDVDCallback ) < 0 )
			{
#				ifdef _DEBUG
				OSReport("*** Failed to read 'FRAM' chunk.\n");
#				endif
				DVDClose( &audio_player.fileHandle );
				audio_player.asyncOpenCallbackStatus = ASYNC_OPEN_FAIL;
				return;
			}
			break;
		}
			
		case 5:
		{
			if( workChunk.id != VID_FCC('F','R','A','M') )
			{
#				ifdef _DEBUG
				OSReport("*** No FRAM chunk found.");
#				endif
				audio_player.asyncOpenCallbackStatus = ASYNC_OPEN_FAIL;
				DVDClose( &audio_player.fileHandle );
				return;
			}

			// 32 bytes of this chunk are already consumed, but we want to 'preload' the NEXT chunk's FRAM header.
			audio_player.nextFrameSize				= workChunk.len; 		
			audio_player.nextFrameOffset			+= 32;

			audio_player.firstFrameOffset			= audio_player.nextFrameOffset;
			audio_player.firstFrameSize				= audio_player.nextFrameSize;

//			strncpy( audio_player.fileName, fileName, 64 );
//			audio_player.fileName[63]				= 0;

			audio_player.open 			 			= TRUE;

			audio_player.asyncOpenCallbackStatus	= ASYNC_OPEN_SUCCESS;
			audio_player.readIndex 					= 0;
			audio_player.decodeIndex 				= 0;
			audio_player.lastDecodedFrame			= 0;
			audio_player.error 			 			= FALSE;
			audio_player.preFetchState 	 			= FALSE;
			audio_player.loopMode		 			= FALSE;
			audio_player.asyncDvdRunning			= FALSE;
			audio_player.currentFrameCount 			= 0;
			audio_player.readBufferBaseMem			= 0;
			break;
		}
	
		default:
		{
			ASSERT( 0 );
			break;
		}
	}
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
BOOL AUDSimpleOpen( char* fileName )
{
//	u32 fileOffset = 0;
//	u32 headSize;
//	u32 audioInfoSize;
	
	if( audio_player.open )
	{
#		ifdef _DEBUG
		OSReport( "*** Cannot open '%s' because audio_player already open.\n", fileName );
#		endif
		return FALSE;
	}

	// Initialise the callback status.
	audio_player.asyncOpenCallbackStatus = 0;
	
	s32 entry_num = DVDConvertPathToEntrynum( fileName );
	if( entry_num == -1 )
	{
#		ifdef _DEBUG
		OSReport( "*** Cannot find '%s'\n", filename );
#		endif
		return FALSE;
	}
	if( DVDFastOpen( entry_num, &audio_player.fileHandle ) == FALSE )
	{
#		ifdef _DEBUG
		OSReport( "*** Cannot open: '%s'\n", fileName );
#		endif
		return FALSE;
	}
		
	// Set callback status to indicate file is now open.
	audio_player.asyncOpenCallbackStatus = 1;
	
	// Read 'VID1' chunk from file and check for correct version.
//	if( DVDRead( &audio_player.fileHandle, &workChunk, 32, 0 ) < 0 )
	if( DVDReadAsync( &audio_player.fileHandle, &workChunk, 32, 0, AUDSimpleOpenDVDCallback ) < 0 )
	{
#		ifdef _DEBUG
		OSReport( "*** Failed to read the header for %s.\n", fileName );
#		endif
		DVDClose( &audio_player.fileHandle );
		return FALSE;
	}

	strncpy( audio_player.fileName, fileName, 64 );
	audio_player.fileName[63]				= 0;

	// Nothing more to do here.
	return TRUE;
		
//	fileOffset += 32;
	
	// Check file id	
//	if( workChunk.id != VID_FCC('V','I','D','1' ))
//	{
//#		ifdef _DEBUG
//		OSReport("*** No VID1 file: '%s'\n", fileName);
//#		endif
//		DVDClose( &audio_player.fileHandle );
//		return FALSE;
//	}
	
	// Check for correct version of vid chunk.
	// If we find this version we assume a 'special' alignment and chunk ordering which may be invalid
	// in another version of the file format.
//	if( workChunk.vid.versionMajor != 1 || workChunk.vid.versionMinor != 0 )
//	{
//#		ifdef _DEBUG
//		OSReport("*** Unsupported file version: major: %d, minor: %d\n", workChunk.vid.versionMajor, workChunk.vid.versionMajor);
//#		endif
//		DVDClose(&audio_player.fileHandle);
//		return FALSE;
//	}
	
//#	ifdef _DEBUG
	// Sometimes, it's required to check for a special build of the VidConv converter.
//	{
//		u32 version = VID_VERSION(workChunk.vid.vidConvMajor, workChunk.vid.vidConvMinor, workChunk.vid.vidConvBuild);
//		if(version < VID_VERSION(1,0,1))
//		OSReport("*** WARNING: Vid file created using an unsupported converter version: %d.%d.%d\n", (u32)workChunk.vid.vidConvMajor, (u32)workChunk.vid.vidConvMinor, (u32)workChunk.vid.vidConvBuild);
//	}
//#	endif

	// Check types of chunks we have in this file.
	// !!! Note that we assume start of 'HEAD' chunk at byte offset 32 from file start !!!
//	if( DVDRead( &audio_player.fileHandle, &workChunk, 32, (s32)fileOffset ) < 0 )
//	{
//#		ifdef _DEBUG
//		OSReport("*** Failed to read 'HEAD' chunk.\n");
//#		endif
//		DVDClose( &audio_player.fileHandle );
//		return FALSE;
//	}
	
//	if( workChunk.id != VID_FCC('H','E','A','D' ))
//	{
//#		ifdef _DEBUG
//		OSReport("*** No HEAD chunk found at expected offset\n");
//#		endif
//		DVDClose(&audio_player.fileHandle);
//		return FALSE;
//	}
	
	// Calculate the start of the first frame chunk
	// (we know the header chunk starts at offset 32)
//	audio_player.nextFrameOffset = workChunk.len + 32;

	// Skip 'HEAD' chunk id, len and version fields
//	fileOffset += VID_CHUNK_HEADER_SIZE;
	
	// The header chunk contains one or more header chunks for the different data types contained
	// in the stream. Parse them all...
//	headSize = workChunk.len - VID_CHUNK_HEADER_SIZE;
//	while( headSize >= 32 )
//	{
//		if( DVDRead( &audio_player.fileHandle, &workChunk, 32, (s32)fileOffset ) < 0 )
//		{
//#			ifdef _DEBUG
//			OSReport("*** Error reading file at offset %d\n", fileOffset);
//#			endif
//			DVDClose(&audio_player.fileHandle);
//			return FALSE;
//		}
		
//		fileOffset += 32;
//		headSize -= 32;

		// We analyze the 1st 32 bytes of the chunk for a known header format
//		if(workChunk.id == VID_FCC('A','U','D','H'))
//		{
			// Allocate memory for audio header chunk
//			audio_player.audioHeaderChunk = (u8*)((*audio_player.cbAlloc)(workChunk.len));
//			audioInfoSize = workChunk.len - VID_CHUNK_HEADER_SIZE;
			
			// Copy the already loaded part
//			memcpy(audio_player.audioHeaderChunk, &workChunk, 32);
//			workChunk.len -= 32;
			
			// Read additional audio header bytes if the audio header is greater that 32 bytes
//			if(workChunk.len >= 32)
//			{
//				ASSERT((workChunk.len&31)==0);
//				if( DVDRead( &audio_player.fileHandle, audio_player.audioHeaderChunk + 32, workChunk.len, (s32)fileOffset ) < 0 )
//				{
//#					ifdef _DEBUG
//					OSReport("*** Error reading file at offset %d\n", fileOffset);
//#					endif
//					DVDClose(&audio_player.fileHandle);
//					return FALSE;
//				}
//				fileOffset += workChunk.len;
//				headSize -= workChunk.len;
//			}

			// Setup and calc the number of bytes which we are allowed to copy into the audioInfo struct
//			memcpy(&audio_player.audioInfo, audio_player.audioHeaderChunk+VID_CHUNK_HEADER_SIZE, MIN(audioInfoSize, sizeof(audio_player.audioInfo)));
//		}
//		else
//		{
//			// Skip unknown chunks. We already read 32 bytes for the header which we must subtract here.
//			fileOffset += workChunk.len - 32;
///			headSize -= workChunk.len - 32;
//		}
//	}
	
	// check if we have the correct vaud file version (>0)
//	if(audio_player.audioInfo.vaudex.version == 0)
//	{
//#		ifdef _DEBUG
//		OSReport("*** Invalid version in vaud file.");
//#		endif
//		DVDClose(&audio_player.fileHandle);
//		return FALSE;
//	}

	// we can only play audio files which have the following fiels set.
	// Note that in case of VIDEO files this fields are allowed to be 0.
//	ASSERT(audio_player.audioInfo.vaudex.maxBufferSize > 0);
//	ASSERT(audio_player.audioInfo.vaudex.frameCount > 0);
//	ASSERT(audio_player.audioInfo.vaudex.frameTimeMs > 0);
	
	// read beginning of 1st frame chunk to get required size information
//	if( DVDRead( &audio_player.fileHandle, &workChunk, 32 , (s32)audio_player.nextFrameOffset ) < 0 )
//	{
//#		ifdef _DEBUG
//		OSReport("*** Failed to read 'FRAM' chunk.\n");
//#		endif
//		DVDClose(&audio_player.fileHandle);
//		return FALSE;
//	}

//	if( workChunk.id != VID_FCC('F','R','A','M') )
//	{
//#		ifdef _DEBUG
//		OSReport("*** No FRAM chunk found.");
//#		endif
//		DVDClose(&audio_player.fileHandle);
//		return FALSE;
//	}

//	audio_player.nextFrameSize = workChunk.len; 		// 32 bytes of this chunk are already consumed, but we want to 'preload' the NEXT chunk's FRAM header
//	audio_player.nextFrameOffset += 32;
	
//	audio_player.firstFrameOffset = audio_player.nextFrameOffset;
//	audio_player.firstFrameSize   = audio_player.nextFrameSize;

//	strncpy(audio_player.fileName, fileName, 64);
//	audio_player.fileName[63] = 0;
	
//	audio_player.open 			 	= TRUE;

//	audio_player.readIndex 			= 0;
//	audio_player.decodeIndex 			= 0;
//	audio_player.lastDecodedFrame		= 0;
//	audio_player.error 			 	= FALSE;
//	audio_player.preFetchState 	 	= FALSE;
//	audio_player.loopMode			 	= FALSE;
//	audio_player.asyncDvdRunning		= FALSE;
//	audio_player.currentFrameCount 	= 0;
//	audio_player.readBufferBaseMem	= 0;

//	return TRUE;
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
BOOL AUDSimpleClose(void)
	{
	if (audio_player.open)
		{
		if (audio_player.preFetchState == FALSE)
			{
			if (!audio_player.asyncDvdRunning)
				{
				audio_player.open = FALSE;
				DVDClose(&audio_player.fileHandle);
				if(audio_player.audioHeaderChunk != NULL)
					{
					(*audio_player.cbFree)(audio_player.audioHeaderChunk);
					audio_player.audioHeaderChunk = NULL;
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
BOOL AUDSimpleDecode(void)
{
	BOOL enabled;
	u8* chunkStart;
	u32 chunkSize;
	u32 frameSize;

	if( audio_player.readBuffer[audio_player.decodeIndex].valid )
	{
		
		// ptr to our (pre-) loaded data INSIDE (!) 'FRAM' chunk
		// (in other words, the 'FRAM' chunk itself is not visible here)
		chunkStart = audio_player.readBuffer[audio_player.decodeIndex].ptr;

		// usually, we read additional 32 bytes for getting info about the NEXT chunk.
		// We only deal with the actual 'FRAM' chunk data here and adjust the size by 32 bytes.
        frameSize = audio_player.readBuffer[audio_player.decodeIndex].size - 32;
		
		// loop across ALL chunks inside 'FRAM'
		while(frameSize >= 32)
		{
			chunkSize = VID_CHUNK_LEN(chunkStart);
			
			if( VID_CHUNK_ID(chunkStart) == VID_FCC('A','U','D','D') )
			{
				// Get the data to the audio system...
				if(! AUDSimpleAudioDecode(chunkStart + VID_CHUNK_HEADER_SIZE, chunkSize - VID_CHUNK_HEADER_SIZE))
				{
#ifdef _DEBUG
					OSReport("*** AUDSimpleAudioDecode failed!\n");
#endif
				}
			}
#ifdef _DEBUG
			else
			{
				OSReport("*** AUDSimpleDecode: unknown chunk type!\n");
			}
#endif
			
			// goto next chunk
			chunkStart += chunkSize;
			frameSize -= chunkSize;
		}
			
		audio_player.lastDecodedFrame = audio_player.readBuffer[audio_player.decodeIndex].frameNumber;
		audio_player.readBuffer[audio_player.decodeIndex].valid = FALSE;
		audio_player.decodeIndex = (audio_player.decodeIndex + 1) % AUD_NUM_READ_BUFFERS;

		// check if loading is still running
		enabled = OSDisableInterrupts();
		if (!audio_player.readBuffer[audio_player.readIndex].valid && !audio_player.asyncDvdRunning)
			ReadFrameAsync();
		OSRestoreInterrupts(enabled);
		
        return TRUE;
	}

#ifdef _DEBUG
	OSReport("*** AUDSimpleDecode: No valid decode buffer found (?).\n");
#endif
	return FALSE;

}



/*!
 ******************************************************************************
 * \brief
 *		Change active play mask
 *
 ******************************************************************************
 */
void AUDSimplePlayMaskChange(void)
	{
	static u32 myPlayMask = 0x3;
	u32 maxChannels = audio_player.audioInfo.vaud.numChannels;
	ASSERT(maxChannels > 0 && maxChannels <= 32);

	myPlayMask <<= 1;
	if(myPlayMask >= ((u32)(1<<maxChannels)))
	   myPlayMask |= 1;

	myPlayMask &= (1<<maxChannels)-1;

	AUDSimpleAudioChangePlayback(&myPlayMask, 1);
	}

/*!
 ******************************************************************************
 * \brief
 *		Get audio sample rate in Hz.
 *
 ******************************************************************************
 */
u32 AUDSimpleGetAudioSampleRate(void)
	{
	return(audio_player.audioInfo.vaud.frq);
	}

/*!
 ******************************************************************************
 * \brief
 *		Return time in ms of one audio frame
 *
 ******************************************************************************
 */
extern u32 AUDSimpleGetFrameTime(void)
	{
	return(audio_player.audioInfo.vaudex.frameTimeMs);
	}

/*!
 ******************************************************************************
 * \brief
 *		Check for drive status
 *
 ******************************************************************************
 */
BOOL AUDSimpleCheckDVDError(void)
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
#endif		// DVDETH

