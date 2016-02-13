/*	Header file functionality...
	.Hed files that describe the contents of .Wad files
	Written by Ken, stolen by Matt*/
#ifndef __HED_H__
#define __HED_H__

namespace File
{

struct SHedFile
{
	// Constants
	enum
	{
		mNO_WAD = 0x80000000
	};

	bool		HasNoWad() const { return FileSize & mNO_WAD; }
	uint32		GetFileSize() const { return FileSize & (~mNO_WAD); }

	union
	{
		uint32 Offset; 			// A SECTOR_SIZE aligned offset of a file within skate3.wad
		char * p_filename; 		// The filename itself (for no_wad heds)
	};

	// The file size, which is the raw file size, not rounded up
	// to a multiple of SECTOR_SIZE.  Highest bit is set if no wad
	// file is associated with it.
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
SHed *LoadHed( const char *filename, bool force_cd = false, bool no_wad = false );

} // namespace File

#endif
