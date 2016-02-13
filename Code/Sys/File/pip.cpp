///////////////////////////////////////////////////////////////////////////////////////
//
// pip.cpp		KSH 6 Feb 2002
//
// Pre-in-Place stuff
//
///////////////////////////////////////////////////////////////////////////////////////

// start autoduck documentation
// @DOC pip
// @module pip | None
// @subindex Scripting Database
// @index script | pip

// TODO
// Use Shrink to shrink the memory block to get rid of the 2048 padding at the end.
// (This will only be effective when the bottom-up heap is being used though)

// TODO but non-essential:
// Make Unload work if passed the pointer.
// Make GetFileSize work if passed the pointer.

// 11Dec02 JCB - Andre wanted me to use tolower() instead of strncmpi()
#include <ctype.h>

#include <sys/file/pip.h>
#include <sys/file/pre.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <sys/file/filesys.h>
#include <core/compress.h>

namespace Pip
{

#define IN_PLACE_DECOMPRESSION_MARGIN 3072


enum EQuadAligned
{
	NOT_QUAD_WORD_ALIGNED=0,
	QUAD_WORD_ALIGNED
};


#define CURRENT_PRE_VERSION	0xabcd0003			// as of 3/14/2001 (and as of 2/12/2002)

struct SPreHeader
{
	int		mSize;
	int		mVersion;
	int		mNumFiles;
};


struct SPreContained
{
	uint32	mDataSize;
	uint32	mCompressedSize;
	uint16	mNameSize;

	// In makepre.cpp, mNameSize is stored in 4 bytes. When the pre is in memory though, I'm
	// borrowing the two high bytes to use as a usage indicator to indicate whether this
	// contained file is 'open', ie has had Load called on it. The count value is the number
	// of times the file has been opened using Load. Gets decremented when Unload is called.
	// (The LoadPre function checks that they are all zero to start with when a new pre is loaded)
	uint16  mUsage;

	// Mick - added space for a checksum of mpName
	// as otherwise we spend over five seconds at boot up in re-calculating checksums n^2 times	
	uint32 	mChecksum;
	
	// Keep mpName the last member of this struct, cos I calculate the space used by the other
	// members by subtracting &p->mDataSize from p->mpName.
	// mpName will probably not move anyway, cos this strcture is part of the pre file format.
	char	mpName[1];
};



static SPreContained *sSkipToNextPreContained(SPreContained *p_preContained, EQuadAligned quadWordAlignedData=QUAD_WORD_ALIGNED);
static SPreHeader 	 *sSkipOverPreName(const char *p_pre_name);
static SPreContained *sSeeIfFileIsInAnyPre(uint32 fileNameCRC);
#ifdef __NOPT_ASSERT__
static const char 	 *sGetPreName(SPreContained *p_contained_file);
#endif

#define MAX_PRE_FILES 100
// char* because each pre is prefixed with the name of the pre.
static char *spp_pre_files[MAX_PRE_FILES];

struct SUnPreedFile
{
	uint8 *mpFileData;
	uint32 mFileNameChecksum;
	uint32 mFileSize;
	uint32 mUsage;
};

#define MAX_UNPREED_FILES 200
static SUnPreedFile sp_unpreed_files[MAX_UNPREED_FILES];

// This class only exists so that I can declare an instance of it and hence
// use its constructor to initialise the above arrays.
// Saves me having to call an init function from somewhere early on in the code,
// which might get moved around later and cause problems.
// This way, the arrays are guaranteed to have got initialised even before main() is
// called. I think ...
class CInit
{
public:
	CInit()
	{
		int i;

		for (i=0; i<MAX_PRE_FILES; ++i)
		{
			spp_pre_files[i]=NULL;
		}

		for (i=0; i<MAX_UNPREED_FILES; ++i)
		{
			sp_unpreed_files[i].mpFileData=NULL;
			sp_unpreed_files[i].mFileNameChecksum=0;
			sp_unpreed_files[i].mFileSize=0;
			sp_unpreed_files[i].mUsage=0;
		}
	}
};
static CInit s_initter;

// Given a pointer to a SPreContained, this will calculate a pointer to the next.
// When used on a source pre quadWordAlignedData should be set to NOT_QUAD_WORD_ALIGNED, since the
// contained files are not aligned in the pre files on disc.
// When used on the processed pre, quadWordAlignedData should be set to QUAD_WORD_ALIGNED, which is its default value.
static SPreContained *sSkipToNextPreContained(SPreContained *p_preContained, EQuadAligned quadWordAlignedData)
{
	Dbg_MsgAssert(p_preContained,("NULL p_preContained"));


	int total_data_size=p_preContained->mDataSize;
	if (p_preContained->mCompressedSize)
	{
		total_data_size=p_preContained->mCompressedSize;
	}
	total_data_size=(total_data_size+3)&~3;

//	printf ("p_preContained = %p\n, total_data = %d, name = (%p) %s",p_preContained,total_data_size,p_preContained->mpName,p_preContained->mpName);
	
	uint32 p_next=(uint32)p_preContained->mpName;
	p_next+=p_preContained->mNameSize;
	if (quadWordAlignedData)
	{
		p_next=(p_next+15)&~15;
	}
	p_next+=total_data_size;

	return (SPreContained*)p_next;
}

static SPreHeader *sSkipOverPreName(const char *p_pre_name)
{
	Dbg_MsgAssert(p_pre_name,("NULL p_pre_name"));

	int len=strlen(p_pre_name)+1; // +1 for terminator
	len=(len+15)&~15;	// Round up to a multiple of 16
	return (SPreHeader*)(p_pre_name+len);
}

// Loads a pre file into memory. The name must have no path since the pre is
// assumed to be in the data\pre directory.
// Ie, a valid name would be "alc.pre"
void LoadPre(const char *p_preFileName)
{
	Dbg_MsgAssert(p_preFileName,("NULL p_preFileName"));

	// Check to see if the pre is already loaded, and return without doing anything if it is.
	for (int i=0; i<MAX_PRE_FILES; ++i)
	{
		if (spp_pre_files[i])
		{
			// Do a case-insensitive comparison.
			if (stricmp(spp_pre_files[i],p_preFileName)==0)
			{
				// Found it! Nothing to do.
				return;
			}
		}
	}

	// Not loaded already, so need to load it.
	// Find a spare slot ...
	int spare_index=-1;
	for (int pre=0; pre<MAX_PRE_FILES; ++pre)
	{
		if (!spp_pre_files[pre])
		{
			spare_index=pre;
			break;
		}
	}
	Dbg_MsgAssert(spare_index!=-1, ("Reached limit of %d pre files loaded at once.",MAX_PRE_FILES));

	// Prefix with 'pre\'
	char p_full_pre_name[500];
	Dbg_MsgAssert(strlen(p_preFileName)<400,("Pre name too long: '%s'",p_preFileName));
	sprintf(p_full_pre_name,"pre\\%s",p_preFileName);

	#ifdef __NOPT_ASSERT__
	printf("Loading pip pre '%s'\n",p_preFileName);
	#endif

	// Calculate the space needed to store the pre name, which will get stuck before the
	// file data.
	int name_size=strlen(p_preFileName)+1;
	name_size=(name_size+15)&~15;



	char *p_old_file_data=NULL;
	uint32 original_file_size = File::CanFileBeLoadedQuickly( p_full_pre_name );
	uint32 old_pre_buffer_size=0;

	if (original_file_size)
	{
		// Goody! It can be loaded quickly.

		old_pre_buffer_size=(original_file_size+2047)&~2047;
		old_pre_buffer_size+=name_size;

		p_old_file_data=(char*)Mem::Malloc(old_pre_buffer_size);
		Dbg_MsgAssert(p_old_file_data,("Could not allocate memory for file %s",p_full_pre_name));

		// Copy in the pre name at the start of the buffer.
		strcpy(p_old_file_data,p_preFileName);

		bool fileLoaded = File::LoadFileQuicklyPlease( p_full_pre_name, (uint8*)(p_old_file_data+name_size) );
		if ( !fileLoaded )
		{
			Dbg_MsgAssert( 0,( "File %s failed to load quickly.\n", p_full_pre_name));
			Mem::Free(p_old_file_data);
			p_old_file_data=NULL;
		}
	}
	else
	{
		// Open the file & get its file size
		void *p_file = File::Open(p_full_pre_name, "rb");

		if (!p_file)
		{
			Dbg_MsgAssert(0,("Could not open file %s",p_full_pre_name));
		}

		original_file_size=File::GetFileSize(p_file);
		if (!original_file_size)
		{
			Dbg_MsgAssert(0,("Zero file size for file %s",p_full_pre_name));
		}


		// Allocate memory.
		// Just to be safe, make sure the buffer is at least the file size rounded up to the
		// next multiple of 2048, cos maybe loading a file off CD will always load
		// whole numbers of sectors.
		// Haven't checked that though.
		old_pre_buffer_size=(original_file_size+2047)&~2047;
		old_pre_buffer_size+=name_size;

		p_old_file_data=(char*)Mem::Malloc(old_pre_buffer_size);
		Dbg_MsgAssert(p_old_file_data,("Could not allocate memory for file %s",p_full_pre_name));

		// Copy in the pre name at the start of the buffer.
		strcpy(p_old_file_data,p_preFileName);

		// Load the file into memory then close the file.
		#ifdef __NOPT_ASSERT__
		long bytes_read=File::Read(p_old_file_data+name_size, 1, original_file_size, p_file);
		Dbg_MsgAssert(bytes_read<=(long)original_file_size,("bytes_read>original_file_size ?"));
		#else
		File::Read(p_old_file_data+name_size, 1, original_file_size, p_file);
		#endif

		File::Close(p_file);
	}


	// Calculate the new buffer size required.
	// Note that even if no decompression is required, the new buffer will probably need to
	// be bigger than the old, because the contained files need to be moved so that they all start
	// on 16 byte boundaries. This is required by the collision code for example.

	uint32 new_pre_buffer_size=name_size;
	new_pre_buffer_size+=sizeof(SPreHeader);

	SPreHeader *p_pre_header=sSkipOverPreName(p_old_file_data);
	uint32 num_files=p_pre_header->mNumFiles;
	SPreContained *p_contained=(SPreContained*)(p_pre_header+1);
	for (uint32 f=0; f<num_files; ++f)
	{
		// Do a quick check to make sure that all the usage values are initially zero.
		// They should be, because the namesize member should never be bigger than 65535 ...
		Dbg_MsgAssert(p_contained->mUsage==0,("The file %s in %s has mUsage=%d ??",p_contained->mpName,p_preFileName,p_contained->mUsage));

		new_pre_buffer_size+=(uint32)p_contained->mpName-(uint32)p_contained;
		new_pre_buffer_size+=p_contained->mNameSize;

		new_pre_buffer_size=(new_pre_buffer_size+15)&~15;

		new_pre_buffer_size+=(p_contained->mDataSize+3)&~3;

		p_contained=sSkipToNextPreContained(p_contained,NOT_QUAD_WORD_ALIGNED);
	}

	// Need to add a small margin to prevent decompressed data overtaking the compressed data.
	new_pre_buffer_size+=IN_PLACE_DECOMPRESSION_MARGIN;

	// At this point we have:
	//
	// original_file_size	The exact size of the original pre file. Will be a multiple of 4.
	//
	// name_size			The size of the pre name, rounded up to a multiple of 4
	//
	// old_pre_buffer_size	The size of the memory buffer pointed to by p_old_file_data
	//						Equals name_size +  (original_file_size, rounded up to a multiple of 2048)
	//
	// new_pre_buffer_size	The required size of the new buffer, to contain the decompressed pre,
	//						with all the files moved to be at 16 byte boundaries.
	//

	#ifdef __NOPT_ASSERT__
	printf("Decompressing and rearranging pre file ...\n");
	#endif

	// Note: It does not matter if new_pre_buffer_size is not a multiple of 2048 any more.
	// old_pre_buffer_size was only a multiple of 2048 to ensure that the file loading
	// did not overrun the end of the buffer. I think file loading may only load a whole
	// number of sectors.


	// Reallocate the buffer.
	char *p_new_file_data=NULL;
	if (Mem::Manager::sHandle().GetContextDirection()==Mem::Allocator::vTOP_DOWN)
	{
		// If using the top-down heap expand the buffer downwards ...

		// p_new_file_data will become a pointer lower down in memory than p_old_file_data.
		// The old data pointed to by p_old_file_data will still be there.
		// The memory between p_new_file_data and p_old_file_data is all free to use. Hoorah!
		p_new_file_data=(char*)Mem::ReallocateDown(new_pre_buffer_size,p_old_file_data);
	}
	else
	{
		// If using the top-down heap expand the buffer upwards ...
		p_new_file_data=(char*)Mem::ReallocateUp(new_pre_buffer_size,p_old_file_data);
		Dbg_MsgAssert(p_new_file_data,("ReallocateUp failed!"));

		// Now need to move the data up so that it can be decompressed into the gap below.
		uint32 *p_source=(uint32*)(p_old_file_data+old_pre_buffer_size);
		uint32 *p_dest=(uint32*)(p_new_file_data+new_pre_buffer_size);
		// Loading backwards cos the destination overlaps the source.

		// Note: Did some timing tests to see if this was was slow, but it's not really.
		// To load all the pre files in the game, one after the other, takes an average of
		// 32.406 seconds when the top down heap is used. When using the bottom up, which
		// necessitates doing this copy, it takes 33.518 seconds.
		// So per pre file that isn't much. (There's about 60 pre files)
		Dbg_MsgAssert((old_pre_buffer_size&3)==0,("old_pre_buffer_size not a multiple of 4 ?"));
		uint32 num_longs=old_pre_buffer_size/4;
		for (uint32 i=0; i<num_longs; ++i)
		{
			--p_source;
			--p_dest;
			*p_dest=*p_source;
		}

		// Now update p_old_file_data to point where it should.
		p_old_file_data=p_new_file_data+new_pre_buffer_size-old_pre_buffer_size;
	}

	// Copy the pre name down.
	for (int i=0; i<name_size; ++i)
	{
		p_new_file_data[i]=p_old_file_data[i];
	}

	// Write in the new pre header
	SPreHeader *p_source_header=(SPreHeader*)(p_old_file_data+name_size);
	SPreHeader *p_dest_header=(SPreHeader*)(p_new_file_data+name_size);
	p_dest_header->mNumFiles=p_source_header->mNumFiles;
	p_dest_header->mVersion=p_source_header->mVersion;

	// Copy down each of the contained files, decompressing those that need it.
	SPreContained *p_source_contained=(SPreContained*)(p_source_header+1);
	SPreContained *p_dest_contained=(SPreContained*)(p_dest_header+1);
	for (uint32 f=0; f<num_files; ++f)
	{
		// Copy down the contained file's header.
		p_dest_contained->mDataSize=p_source_contained->mDataSize;
		p_dest_contained->mChecksum=p_source_contained->mChecksum;
		p_dest_contained->mCompressedSize=0; // The new file will not be compressed.
		p_dest_contained->mUsage=0;
		p_dest_contained->mNameSize=p_source_contained->mNameSize;
		for (int i=0; i<p_source_contained->mNameSize; ++i)
		{
			p_dest_contained->mpName[i]=p_source_contained->mpName[i];
		}

		// Pre-calculate p_next_source_contained because decompression may (and often will)
		// cause the new data to overwrite the contents of p_source_contained.
		SPreContained *p_next_source_contained=sSkipToNextPreContained(p_source_contained,NOT_QUAD_WORD_ALIGNED);

		uint8 *p_source=(uint8*)(p_source_contained->mpName+p_source_contained->mNameSize);
		uint8 *p_dest=(uint8*)(p_dest_contained->mpName+p_dest_contained->mNameSize);
		p_dest=(uint8*)( ((uint32)p_dest+15)&~15 );

		if (p_source_contained->mCompressedSize)
		{
			uint32 num_bytes_decompressed=p_dest_contained->mDataSize;
			uint8 *p_end=DecodeLZSS(p_source,p_dest,p_source_contained->mCompressedSize);
			Dbg_MsgAssert(p_end==p_dest+num_bytes_decompressed,("Eh? DecodeLZSS wrote %d bytes, expected it to write %d",p_end-p_dest,num_bytes_decompressed));

			// For neatness, write zero's into the pad bytes at the end, otherwise they'll
			// be uninitialised data.
			while (num_bytes_decompressed & 3)
			{
				*p_end++=0;
				++num_bytes_decompressed;
			}
		}
		else
		{
			// Uncompressed, so just copy the data down.
			uint32 *p_source_long=(uint32*)p_source;
			uint32 *p_dest_long=(uint32*)p_dest;
			// mDataSize is not necessarily a multiple of 4, but the actual data will be
			// padded at the end so that it does occupy a whole number of long words.
			// So the +3 is to ensure that n is rounded up to the next whole number of longs.
			uint32 n=(p_source_contained->mDataSize+3)/4;

			for (uint32 i=0; i<n; ++i)
			{
				*p_dest_long++=*p_source_long++;
			}
		}

		p_source_contained=p_next_source_contained;

		p_dest_contained=sSkipToNextPreContained(p_dest_contained);
	}

	// I don't think I ever need to reference mSize again, but might as well make it correct.
	p_dest_header->mSize=(uint32)p_dest_contained-(uint32)p_dest_header;

	//printf("Wasted space = %d\n",new_pre_buffer_size-((uint32)p_dest_contained-(uint32)p_new_file_data));

	spp_pre_files[spare_index]=p_new_file_data;
	#ifdef __NOPT_ASSERT__
	printf("Done\n");
	#endif
}

// Removes the specified pre from memory. The name must have no path since the pre is
// assumed to be in the data\pre directory.
// Ie, a valid name would be "alc.pre"
//
// Won't do anything if the pre is gone already.
// Asserts if any of the files in the pre are still 'open' in that they haven't had Unload called.
bool UnloadPre(const char *p_preFileName)
{
	bool success = false;

	Dbg_MsgAssert(p_preFileName,("NULL p_preFileName"));

	for (int i=0; i<MAX_PRE_FILES; ++i)
	{
		if (spp_pre_files[i])
		{
			// Do a case-insensitive comparison.
			if (stricmp(spp_pre_files[i],p_preFileName)==0)
			{
				// Found it!

				// Before unloading it, make sure none of the files within is still open.
				#ifdef __NOPT_ASSERT__
				SPreHeader *p_pre_header=sSkipOverPreName(spp_pre_files[i]);
				int num_files=p_pre_header->mNumFiles;
				SPreContained *p_contained=(SPreContained*)(p_pre_header+1);
				for (int f=0; f<num_files; ++f)
				{
					Dbg_MsgAssert(!p_contained->mUsage,("Tried to unload %s when the file %s contained within it was still open! (mUsage=%d)",spp_pre_files[i],p_contained->mpName,p_contained->mUsage));
					p_contained=sSkipToNextPreContained(p_contained);
				}
				#endif

				// Delete it.
				Mem::Free(spp_pre_files[i]);
				spp_pre_files[i]=NULL;

				// we've successfully unloaded a pre
				success = true;
			}
		}
	}

	// Not found, so nothing to do.
	return success;
}

// Searches each of the loaded pre files for the passed contained file.
// Returns NULL if not found.
static SPreContained *sSeeIfFileIsInAnyPre(uint32 fileNameCRC)
{
	for (int i=0; i<MAX_PRE_FILES; ++i)
	{
		if (spp_pre_files[i])
		{
			SPreHeader *p_pre_header=sSkipOverPreName(spp_pre_files[i]);
			int num_files=p_pre_header->mNumFiles;
			SPreContained *p_contained=(SPreContained*)(p_pre_header+1);
			for (int f=0; f<num_files; ++f)
			{
				//Dbg_MsgAssert(Crc::GenerateCRCFromString(p_contained->mpName) == p_contained->mChecksum,
				//("Checksum for %s (%x) not %x",p_contained->mpName,Crc::GenerateCRCFromString(p_contained->mpName),p_contained->mChecksum));
				//if ( Crc::GenerateCRCFromString(p_contained->mpName) == fileNameCRC )
				if ( p_contained->mChecksum == fileNameCRC )
				{
					return p_contained;
				}
				p_contained=sSkipToNextPreContained(p_contained);
			}
		}
	}
	return NULL;
}

#ifdef __NOPT_ASSERT__
// Finds which pre the passed file is contained in, and returns a pointer to the pre name.
// Returns "Unknown" if not found anywhere.
static const char *sGetPreName(SPreContained *p_contained_file)
{
	for (int i=0; i<MAX_PRE_FILES; ++i)
	{
		if (spp_pre_files[i])
		{
			SPreHeader *p_pre_header=sSkipOverPreName(spp_pre_files[i]);
			int num_files=p_pre_header->mNumFiles;
			SPreContained *p_contained=(SPreContained*)(p_pre_header+1);
			for (int f=0; f<num_files; ++f)
			{
				if (p_contained==p_contained_file)
				{
					return spp_pre_files[i];
				}
				p_contained=sSkipToNextPreContained(p_contained);
			}
		}
	}

	return "Unknown";
}
#endif

void* Load(const char* p_fileName)
{
	uint32 filename_checksum=Crc::GenerateCRCFromString(p_fileName);
	
	// First, see if the file is in one of the loaded pre files.
	SPreContained *p_contained_file=sSeeIfFileIsInAnyPre( filename_checksum );
	if (p_contained_file)
	{
		++p_contained_file->mUsage;

		Dbg_MsgAssert(p_contained_file->mCompressedSize==0,("The file '%s' is stored compressed in %s !",p_fileName,sGetPreName(p_contained_file)));

		uint32 p_data=(uint32)p_contained_file->mpName+p_contained_file->mNameSize;
		p_data=(p_data+15)&~15;
		return (void*)p_data;
	}

	// Next, see if it is one of the unpreed files that is already loaded.
	for (int i=0; i<MAX_UNPREED_FILES; ++i)
	{
		if (sp_unpreed_files[i].mpFileData)
		{
			if (sp_unpreed_files[i].mFileNameChecksum==filename_checksum)
			{
				// It is already loaded, so increment the usage and return the pointer.
				++sp_unpreed_files[i].mUsage;

				Dbg_MsgAssert(sp_unpreed_files[i].mpFileData,("NULL sp_unpreed_files[i].mpFileData ?"));
				return sp_unpreed_files[i].mpFileData;
			}
		}
	}

	// It's not in a pre, and it is not already loaded, so load it.

	// Find a free slot.
	SUnPreedFile *p_new_unpreed_file=NULL;
	for (int i=0; i<MAX_UNPREED_FILES; ++i)
	{
		if (!sp_unpreed_files[i].mpFileData)
		{
			p_new_unpreed_file=&sp_unpreed_files[i];
			break;
		}
	}
	Dbg_MsgAssert(p_new_unpreed_file,("Too many unpreed files open at once! Max=%d (MAX_UNPREED_FILES in pip.cpp)",MAX_UNPREED_FILES));

	// allocate memory, and load the file
	int	file_size;
	uint8 * p_file_data = (uint8*) File::LoadAlloc(p_fileName,&file_size);
	Dbg_MsgAssert(p_file_data,("Failsed to load %s\n",p_fileName));

	// Fill in the slot.
	p_new_unpreed_file->mpFileData=p_file_data;
	p_new_unpreed_file->mFileSize=file_size;

	p_new_unpreed_file->mFileNameChecksum=filename_checksum;
	p_new_unpreed_file->mUsage=1;

	return p_file_data;
}

void Unload(uint32 fileNameCRC)
{
	// See if it is one of the unpreed files.
	#ifdef __NOPT_ASSERT__
	bool is_an_unpreed_file=false;
	#endif
	for (int i=0; i<MAX_UNPREED_FILES; ++i)
	{
		if (sp_unpreed_files[i].mpFileData)
		{
			if ( sp_unpreed_files[i].mFileNameChecksum == fileNameCRC )
			{
				// Decrement the usage
				if (sp_unpreed_files[i].mUsage)
				{
					--sp_unpreed_files[i].mUsage;
				}

				// Free up the memory if nothing is using this file any more.
				if (sp_unpreed_files[i].mUsage==0)
				{
					Mem::Free(sp_unpreed_files[i].mpFileData);
					sp_unpreed_files[i].mpFileData=NULL;

					sp_unpreed_files[i].mFileSize=0;
					sp_unpreed_files[i].mFileNameChecksum=0;
					sp_unpreed_files[i].mUsage=0;
				}

				#ifdef __NOPT_ASSERT__
				is_an_unpreed_file=true;
				#endif
			}
		}
	}

	SPreContained *p_contained_file=sSeeIfFileIsInAnyPre(fileNameCRC);
	if (p_contained_file)
	{
		Dbg_MsgAssert(!is_an_unpreed_file,("Tried to unload '%s' which was loaded unpreed, but also exists in %s ?",p_contained_file->mpName,sGetPreName(p_contained_file)));

		// Decrement the usage.
		if (p_contained_file->mUsage)
		{
			--p_contained_file->mUsage;
		}
	}
}

// If the file is unpreed, this will decrement the usage and free the memory specially allocated
// for it if the usage has reached zero.
// If the file is in a loaded pre, it will decrement that files usage in the pre.
// If the file is both of the above, it will assert.
// If it is neither, it won't do anything.
void Unload(const char *p_fileName)
{
	Dbg_MsgAssert(p_fileName,("NULL p_fileName"));

	Unload( Crc::GenerateCRCFromString(p_fileName) );
}

uint32 GetFileSize(uint32 fileNameCRC)
{
	// See if it is one of the unpreed files.
	#ifdef __NOPT_ASSERT__
	bool is_an_unpreed_file=false;
	#endif
	for (int i=0; i<MAX_UNPREED_FILES; ++i)
	{
		if (sp_unpreed_files[i].mpFileData)
		{
			if ( sp_unpreed_files[i].mFileNameChecksum == fileNameCRC )
			{
				Dbg_MsgAssert(sp_unpreed_files[i].mFileSize,("Zero mFileSize ??"));
				return sp_unpreed_files[i].mFileSize;

				#ifdef __NOPT_ASSERT__
				is_an_unpreed_file=true;
				#endif
			}
		}
	}

	SPreContained *p_contained_file=sSeeIfFileIsInAnyPre( fileNameCRC );
	if (p_contained_file)
	{
		Dbg_MsgAssert(!is_an_unpreed_file,("'%s' is both unpreed, and also exists in %s ?",p_contained_file->mpName,sGetPreName(p_contained_file)));
		Dbg_MsgAssert(p_contained_file->mDataSize,("Zero mDataSize ??"));
		return p_contained_file->mDataSize;
	}

//	Dbg_MsgAssert(0,("File '%s' not found loaded anywhere",p_fileName));
	return 0;
}

uint32 GetFileSize(const char *p_fileName)
{
	Dbg_MsgAssert(p_fileName,("NULL p_fileName"));

	uint32 file_size = GetFileSize( Crc::GenerateCRCFromString(p_fileName) );

	if ( file_size == 0 )
	{
		Dbg_MsgAssert(0,("File '%s' not found loaded anywhere",p_fileName));
	}

	return file_size;
}

const char *GetNextLoadedPre(const char *p_pre_name)
{
	if (p_pre_name==NULL)
	{
		for (int i=0; i<MAX_PRE_FILES; ++i)
		{
			if (spp_pre_files[i])
			{
				return spp_pre_files[i];
			}
		}
		return NULL;
	}

	bool found=false;
	for (int i=0; i<MAX_PRE_FILES; ++i)
	{
		if (spp_pre_files[i])
		{
			if (found)
			{
				return spp_pre_files[i];
			}
			// Do a case-insensitive comparison.
			if (stricmp(spp_pre_files[i],p_pre_name)==0)
			{
				found=true;
			}
		}
	}

	return NULL;
}

bool PreFileIsInUse(const char *p_pre_name)
{
	Dbg_MsgAssert(p_pre_name,("NULL p_pre_name"));

	for (int i=0; i<MAX_PRE_FILES; ++i)
	{
		if (spp_pre_files[i])
		{
			// Do a case-insensitive comparison.
			if (stricmp(spp_pre_files[i],p_pre_name)==0)
			{
				// Found it!

				SPreHeader *p_pre_header=sSkipOverPreName(spp_pre_files[i]);
				int num_files=p_pre_header->mNumFiles;
				SPreContained *p_contained=(SPreContained*)(p_pre_header+1);
				for (int f=0; f<num_files; ++f)
				{
					if (p_contained->mUsage)
					{
						return true;
					}
					p_contained=sSkipToNextPreContained(p_contained);
				}
			}
		}
	}

	return false;
}

// @script | LoadPipPre |
// @uparm "string" | filename
// @parmopt name | heap | 0 (checksum value) | Which heap to use.
// Possible values are TopDown or BottomUp
// If no heap is specified it will use whatever the current heap is.
bool ScriptLoadPipPre(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_filename;
	pParams->GetString(NONAME, &p_filename, Script::ASSERT);

	uint32 chosen_heap=0;
	pParams->GetChecksum("Heap",&chosen_heap);
	switch (chosen_heap)
	{
		case 0:
			chosen_heap = 0;
			// Use whatever the current heap is.
			break;
		case 0x477fc6de: // TopDown
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
			break;
		case 0xc80bf12d: // BottomUp
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
			break;
		default:
			Dbg_Warning("Heap '%s' not supported by LoadPipPre, using current heap instead.",Script::FindChecksumName(chosen_heap));
			chosen_heap=0;
			break;
	}

	LoadPre(p_filename);

	if (chosen_heap)
	{
		Mem::Manager::sHandle().PopContext();
	}

	return true;
}

// @script | UnLoadPipPre |
// @uparm "string" | filename
bool ScriptUnloadPipPre(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_filename;
	pParams->GetString(NONAME, &p_filename, Script::ASSERT);

	return UnloadPre(p_filename);
}

// @script | DumpPipPreStatus | Prints the status of the currently loaded pre files.
// @flag ShowPreedFiles | Lists all the files contained within each pre, together with their usage value.
// @flag ShowUnPreedFiles | Lists all the files currently open which were not in a pre,
// so were individually opened.
// @flag ShowOnlyOpenFiles | When listing the files in each pre, only display those whose usage is
// greater than zero.
bool ScriptDumpPipPreStatus(Script::CStruct *pParams, Script::CScript *pScript)
{
	bool show_preed_files=pParams->ContainsFlag("ShowPreedFiles");
	bool show_unpreed_files=pParams->ContainsFlag("ShowUnPreedFiles");
	bool show_only_open_files=pParams->ContainsFlag("ShowOnlyOpenFiles");

	printf("Currently loaded pre files:\n");

	for (int i=0; i<MAX_PRE_FILES; ++i)
	{
		if (spp_pre_files[i])
		{
			printf("### %s ###\n",spp_pre_files[i]);

			if (show_preed_files)
			{
				SPreHeader *p_pre_header=sSkipOverPreName(spp_pre_files[i]);
				int num_files=p_pre_header->mNumFiles;
				SPreContained *p_contained=(SPreContained*)(p_pre_header+1);
				for (int f=0; f<num_files; ++f)
				{
					if (!show_only_open_files || p_contained->mUsage)
					{
						printf("Usage:%d  File: %s\n",p_contained->mUsage,p_contained->mpName);
						// Delay so that printf does not break when there are lots of files.
						for (volatile int pp=0; pp<100000; ++pp);
					}

					p_contained=sSkipToNextPreContained(p_contained);
				}
			}
		}
	}

	if (show_unpreed_files)
	{
		printf("Currently loaded un-preed files:\n");

		for (int i=0; i<MAX_UNPREED_FILES; ++i)
		{
			if (sp_unpreed_files[i].mpFileData)
			{
				printf("Usage:%d  File: %s\n",sp_unpreed_files[i].mUsage,Script::FindChecksumName(sp_unpreed_files[i].mFileNameChecksum));
			}
		}
	}

	return true;
}

}

////////////////////////////////////////////////////////////////////////////////
// File:: functions
// Should really be in filesys.cpp, but there is none!!!!

namespace File
{


// Load file as quickly as possible
// Try loading from the pre manager first, then from the CD, using the "quick filesys"
// then finally using the regular file loading functions
// returns pointer to allocated memory, or NULL if file fails to load
// optionally returns size of file in bytes, in *p_filesize (ignored if p_filesize is NULL)
// if file fails to load, *p_filesize will be 0
// if you pass in p_dest, you must also pass in maxSize, the size of the buffer
void * LoadAlloc(const char *p_fileName, int *p_filesize, void *p_dest, int maxSize)
{

	// Mick 2/19/2003 - Removed code that stripped project specific headers
	// as this is now handled at the gs_file level
	
	int	file_size = 0;
// Perhaps the file is in a PRE file,  so try loading it directly, as that will be quickest
	uint8 *p_file_data = (uint8*)File::PreMgr::Instance()->LoadFile(p_fileName,&file_size, p_dest);
	if (!p_file_data)
	{
		// nope, so try loading "Quickly" (basically if we are on a CD)
		file_size = File::CanFileBeLoadedQuickly( p_fileName );
		if (file_size)
		{
			// It can be loaded quickly.
			if (!p_dest)
			{
				p_file_data=(uint8*)Mem::Malloc((file_size+2047)&~2047);
			}
			else
			{
				p_file_data = (uint8*)p_dest;
			}

			bool fileLoaded = File::LoadFileQuicklyPlease( p_fileName, p_file_data );
			if ( !fileLoaded )
			{
				Dbg_MsgAssert( 0,( "File %s failed to load quickly.\n", p_fileName));
				if (!p_dest)
				{
					Mem::Free(p_file_data);
				}
				p_file_data=NULL;
			}
		}
		else
		{
			// can't load quickly, so probably loading from the PC
			// Open the file & get its file size

			void *p_file = File::Open(p_fileName, "rb");
			if (!p_file)
			{
				Dbg_MsgAssert(0,("Could not open file '%s'",p_fileName));
			}

			file_size=File::GetFileSize(p_file);
			if (!file_size)
			{
				Dbg_MsgAssert(0,("Zero file size for file %s",p_fileName));
			}

			if (!p_dest)
			{
				// Allocate memory.
				// Just to be safe, allocate a buffer of size file_size rounded up to the
				// next multiple of 2048, cos maybe loading a file off CD will always load
				// whole numbers of sectors.
				// Haven't checked that though.
				p_file_data=(uint8*)Mem::Malloc((file_size+2047)&~2047);
				Dbg_MsgAssert(p_file_data,("Could not allocate memory for file %s",p_fileName));
			}
			else
			{
				p_file_data = (uint8*)p_dest;
			}
			// Load the file into memory then close the file.
			#ifdef __NOPT_ASSERT__
			long bytes_read=File::Read(p_file_data, 1, file_size, p_file);
			Dbg_MsgAssert(bytes_read<=file_size,("bytes_read>file_size ?"));
			#else
			File::Read(p_file_data, 1, file_size, p_file);
			#endif

			File::Close(p_file);
		}
		// Shrink memory back down to accurate usage - saves 43K total in the school!!!
		if (!p_dest && p_file_data)
		{
			Mem::ReallocateShrink(file_size,p_file_data);
		}
	}
	if (p_filesize)
	{
		*p_filesize = file_size;
	}
	// If we specified a destination, then make sure we did not overflow it
	if (p_dest)
	{
		Dbg_MsgAssert(((file_size + 2047)&~2047) < maxSize,("file size (%d) overflows buffer (%d) for %s\n",file_size,maxSize,p_fileName));
	}
	return (void *)p_file_data;
}


}



