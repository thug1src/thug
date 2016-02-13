/*	Header file functionality...
	.Hed files that describe the contents of .Wad files
	Written by Ken, stolen by Matt*/
#ifndef __HED_H__
#define __HED_H__

namespace File
{

struct SHed
{
	// A SECTOR_SIZE aligned offset of a file within skate3.wad
	uint32 Offset;
	
	// The file size, which is the raw file size, not rounded up
	// to a multiple of SECTOR_SIZE
	uint32 FileSize;
	
	// The filename, which is actually bigger than one byte, tee hee.
	const char pFilename[1];
};


SHed *FindFileInHed(const char *pFilename, SHed *pHed );
SHed *FindFileInHedUsingChecksum( uint32 checksum, SHed *pHed, bool stripPath );
SHed *LoadHed( const char *filename );

} // namespace File

#endif
