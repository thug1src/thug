/*	Header file functionality...
	.Hed files that describe the contents of .Wad files
	Written by Ken, stolen by Matt*/
#ifndef __HED_H__
#define __HED_H__

namespace File
{

struct SHedFile
{
	// A SECTOR_SIZE aligned offset of a file within skate3.wad
	uint32 Offset;
	
	// The file size, which is the raw file size, not rounded up
	// to a multiple of SECTOR_SIZE
	uint32 FileSize;
	
	// Filename checksum (does not include directory).
	uint32 Checksum;
};

struct SHed
{
	// Number of files in this directory.
	uint32 numFiles;
	
	// Checksum of this directory.
	uint32 Checksum;

	// Pointer to File list.
	SHedFile * p_fileList;

	// The filename, which is actually bigger than one byte, tee hee.
//	const char pFilename[1];
};


SHedFile *FindFileInHed(const char *pFilename, SHed *pHed );
SHedFile *FindFileInHedUsingChecksum( uint32 checksum, SHed *pHed );
SHed *LoadHed( const char *filename );

} // namespace File

#endif


