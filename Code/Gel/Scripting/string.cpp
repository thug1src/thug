///////////////////////////////////////////////////////////////////////////////////////
//
// string.cpp		KSH 17 Oct 2001
//
// Code for managing allocation and deallocation of strings within the script system.
// For example it manages the 'permanent string' heap, which is for strings that get created
// when all the qb's are loaded and never get deleted. They can therefore all be squashed
// together for optimum use of memory.
//
// Note: Not defining a CString class, because it would have to have a char pointer so
// would just be an unneccessary layer of indirection that would waste space.
//
// Instead CreateString and DeleteString are used to allocate and deallocate strings.
//
// Could add cleverer memory management here later, eg a pool of fixed sized strings
// for strings that need to be alloc'd and dealloc'd fast.
//
///////////////////////////////////////////////////////////////////////////////////////

// Note: Small memory saving possible by deleting the sp_permanent_string_checksums array
// once you know that no more permanent strings will need to be allocated. Will save 32K ish,
// depending on maxStrings.
// (There's no function here yet for doing that, but easy to add)

#include <gel/scripting/string.h>
#ifndef __CORE_CRC_H
#include <core/crc.h>
#endif

namespace Script
{

static char *sAddToSpecialStringHeap(const char *p_string);

static bool s_use_permanent_string_heap=false;

// The permanent string buffer.
static char *sp_permanent_string_heap=NULL;
static char *sp_permanent_string_heap_top=NULL;
static uint32 s_permanent_string_heap_size=0;

// Array of string checksums to allow quick checking to see if a particular string
// is already in the permanent string buffer without having to do string comparisons.
// Needed because when loading several thousand strings, several million string comparisons
// would have to be done, causing a significant pause in loading the game.
struct SSpecialStringChecksum
{
	uint32 mChecksum;
	char *mpString;
};
static SSpecialStringChecksum *sp_permanent_string_checksums=NULL;
static uint32 s_num_permanent_string_checksums=0;
static uint32 s_max_permanent_string_checksums=0;

// Needs to be called once at the start of the game.
// For THPS3, maxSize was 48000 for English, 60500 for other languages. maxStrings was 4000
void AllocatePermanentStringHeap(uint32 maxSize, uint32 maxStrings)
{
	// Uses whatever the current heap is set to, which is set to the script heap in Script::AllocatePools
	// which is where this function is called from.
	s_permanent_string_heap_size=maxSize;
	s_max_permanent_string_checksums=maxStrings;
	
	// Allocate the string buffer.
	Dbg_MsgAssert(sp_permanent_string_heap==NULL,("sp_permanent_string_heap not NULL ???"));
	sp_permanent_string_heap=(char*)Mem::Malloc(s_permanent_string_heap_size);
	sp_permanent_string_heap_top=sp_permanent_string_heap;
	
	// Allocate the array of checksums.
	Dbg_MsgAssert(sp_permanent_string_checksums==NULL,("sp_permanent_string_checksums not NULL ???"));
	sp_permanent_string_checksums=(SSpecialStringChecksum*)Mem::Malloc(s_max_permanent_string_checksums*sizeof(SSpecialStringChecksum));
	s_num_permanent_string_checksums=0;
}

void DeallocatePermanentStringHeap()
{
	// Deallocate the string buffer.
	Dbg_MsgAssert(sp_permanent_string_heap,("NULL sp_permanent_string_heap ?"));
	Mem::Free(sp_permanent_string_heap);
	sp_permanent_string_heap=NULL;
	sp_permanent_string_heap_top=NULL;
	s_permanent_string_heap_size=0;
	
	// Deallocate the array of checksums.
	// MEMOPT: TODO: Destroy this when UseRegularStringHeap is called too, to free up memory ?
	Dbg_MsgAssert(sp_permanent_string_checksums,("NULL sp_permanent_string_checksums ?"));
	Mem::Free(sp_permanent_string_checksums);
	sp_permanent_string_checksums=NULL;
	s_num_permanent_string_checksums=0;
	s_max_permanent_string_checksums=0;
}

// Adds the contents of the passed pString to the special string heap, and returns
// the pointer to the string within the heap.
// If the string is already found in the heap, it won't add it again.
static char *sAddToSpecialStringHeap(const char *p_string)
{
	Dbg_MsgAssert(p_string,("NULL p_string"));

	// Check to see if it already exists in the special string heap ...

	// Calculate the checksum of the string, then search for this in the array of
	// checksums of all the special strings so far.
	// Quicker than doing byte comparisons, probably due to the long time it takes to
	// read single bytes from memory.
	// Needs to be fast, otherwise it can add a significant delay to the startup time.
	// This method adds about 0.3 seconds altogether (in THPS3, couple of thousand strings)
	uint32 len=strlen(p_string);
	uint32 checksum=Crc::GenerateCRCCaseSensitive(p_string,len);
	
	Dbg_MsgAssert(sp_permanent_string_checksums,("NULL sp_permanent_string_checksums ??"));
	SSpecialStringChecksum *p_ch=sp_permanent_string_checksums;
	for (uint32 i=0; i<s_num_permanent_string_checksums; ++i)
	{
		if (p_ch->mChecksum==checksum)
		{
			return p_ch->mpString;
		}
		++p_ch;
	}		

	char *p_heap_string=NULL;
	
	// Not found, so add to the pile of strings.
	Dbg_MsgAssert(sp_permanent_string_heap,("NULL sp_permanent_string_heap ??"));
	
	Dbg_MsgAssert(s_permanent_string_heap_size-(sp_permanent_string_heap_top-sp_permanent_string_heap)>=len+1,("Out of special string heap"));
	p_heap_string=sp_permanent_string_heap_top;
	strcpy(p_heap_string,p_string);
	// Update sp_permanent_string_heap_top
	sp_permanent_string_heap_top+=len;
	++sp_permanent_string_heap_top; // Skip over the terminator

	// Store the checksum too.
	Dbg_MsgAssert(s_num_permanent_string_checksums<s_max_permanent_string_checksums,("Ran out of permanent-string checksums"));
	sp_permanent_string_checksums[s_num_permanent_string_checksums].mChecksum=checksum;
	sp_permanent_string_checksums[s_num_permanent_string_checksums].mpString=p_heap_string;
	++s_num_permanent_string_checksums;
	
	return p_heap_string;
}

void UsePermanentStringHeap()
{
	s_use_permanent_string_heap=true;
}

void UseRegularStringHeap()
{
	s_use_permanent_string_heap=false;

	// 56K memory saving, though this assumes that UsePermanentStringHeap() won't be called
	// again after UseRegularStringHeap(). If it is, the code will assert as soon as a string
	// is attempted to be created.
	// The current code does not use the permanent string heap again, so use the savings.
	// (End of THPS6 and memory is tight)
	if (sp_permanent_string_checksums)
	{
		Mem::Free(sp_permanent_string_checksums);
		sp_permanent_string_checksums=NULL;
	}	
}

// Allocates space for the passed string and copies it in, and returns a pointer to the new string,
// so the original string can safely be deleted.
// I guess this should really return a const char*, but if it did then DeleteString would
// have to take a const char* too, and it can't because then it would not be able to call
// Mem::Free on it.
char *CreateString(const char *p_string)
{
	Dbg_MsgAssert(p_string,("NULL p_string"));
	
	if (s_use_permanent_string_heap)
	{
		return sAddToSpecialStringHeap(p_string);
	}

	// Just do a regular allocation.	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());

	char *p_new_string=(char*)Mem::Malloc(strlen(p_string)+1); // +1 for terminator
	strcpy(p_new_string,p_string);
	
	Mem::Manager::sHandle().PopContext();
	
	return p_new_string;
}

void DeleteString(char *p_string)
{
	Dbg_MsgAssert(p_string,("NULL p_string"));

	// Check whether the string is in the permanent-string buffer.
	Dbg_MsgAssert(sp_permanent_string_heap,("NULL sp_permanent_string_heap ?"));
	if (p_string>=sp_permanent_string_heap && p_string<sp_permanent_string_heap+s_permanent_string_heap_size)
	{
		// It is in the permanent-string buffer, so nothing to do.
	}
	else
	{
		// Must have been allocated the usual way, so free it.
		Mem::Free(p_string);
	}
}

/*
#ifdef __NOPT_ASSERT__
const char *FindPermanentStringWithChecksum(uint32 ch)
{
	const char *p_string=sp_permanent_string_heap;
	while (p_string < sp_permanent_string_heap_top)
	{
		//printf("%s\n",p_string);
		if (Crc::GenerateCRCFromString(p_string) == ch)
		{
			return p_string;
		}
		p_string=p_string+strlen(p_string)+1; // +1 for the terminating 0
	}
	return NULL;
}
#endif
*/
	
// Mick's script strings
#define	MAX_SCRIPT_STRING		1
#define	SCRIPT_STRING_LENGTH 	128
																  
static char sp_script_string[MAX_SCRIPT_STRING][SCRIPT_STRING_LENGTH]={{0}};

// given a string
void SetScriptString(uint32 n, const char *p_string)
{
	Dbg_MsgAssert(n<MAX_SCRIPT_STRING,("Bad index of %d sent to SetScriptString, MAX_SCRIPT_STRING=%d",n,MAX_SCRIPT_STRING));
	Dbg_MsgAssert(p_string,("NULL p_string"));
	
	sprintf (sp_script_string[n],p_string);	
	Dbg_MsgAssert(strlen(sp_script_string[n])<SCRIPT_STRING_LENGTH,("Script String %d too big\n%s",n,sp_script_string[n]));
}

char* GetScriptString(uint32 n)
{
	Dbg_MsgAssert(n<MAX_SCRIPT_STRING,("Bad index of %d sent to GetScriptString, MAX_SCRIPT_STRING=%d",n,MAX_SCRIPT_STRING));
	
	Dbg_MsgAssert(strlen(sp_script_string[n])<SCRIPT_STRING_LENGTH,("Script String %d too big\n%s",n,sp_script_string[n]));
	return sp_script_string[n];
}


} // namespace Script
