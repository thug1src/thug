///////////////////////////////////////////////////////////////////////////////////////
//
// checksum.cpp		KSH 22 Oct 2001
//
// Checksum name lookup stuff for use by asserts.
// Uses up a heck of a lot of memory, but only when __NOPT_ASSERT__ is defined.
//
///////////////////////////////////////////////////////////////////////////////////////

#ifdef __PLAT_WN32__
#ifdef __QDEBUG__
#include <../tools/src/monitor/stdafx.h> // Needed for the message box
#endif
#endif

#include <gel/scripting/checksum.h>
#include <string.h>	// For stricmp
#include <core/crc.h>
#include <sys/config/config.h>
#include <gel/scripting/debugger.h>

// These are only referenced in the Playstation version.
// They are set in NGPS.lk
#ifdef __PLAT_NGPS__
extern char _script_debugger_start[];
extern char _script_start[];
extern char _script_end[];
#endif

namespace Script
{

// The number of bits is set to be high to speed up AddChecksumName, so that
// LoadQB does not take ages to execute.
#define CHECKSUM_LOOKUP_HASH_BITS 16
// This probably does not need to be so big, but we've got lots of memory in debug so it's OK.
#define CHECKSUM_ENTRIES_PER_SLOT 10

// There are (1<<CHECKSUM_LOOKUP_HASH_BITS) * CHECKSUM_ENTRIES_PER_SLOT of these,
// which uses about 5.2Meg.
struct SChecksumName
{
	uint32 mChecksum;
	const char *mpName;
};

// The space available to hold all the checksum names. The names are stored contiguously
// with no wasted space in between. They stay in memory forever.
#define CHECKSUM_NAME_SPACE 5000000

static SChecksumName *sp_checksum_name_hash_table=NULL;
static char *sp_checksum_names=NULL;
static char *sp_end_of_checksum_names=NULL;

void AllocateChecksumNameLookupTables()
{
	if (!Config::GotExtraMemory())
	{
		return;
	}
		
	Dbg_MsgAssert(sp_checksum_name_hash_table==NULL,("sp_checksum_name_hash_table not NULL ?"));				  
	Dbg_MsgAssert(sp_checksum_names==NULL,("sp_checksum_names not NULL ?"));				  
	Dbg_MsgAssert(sp_end_of_checksum_names==NULL,("sp_end_of_checksum_names not NULL ?"));				  
	
	#ifdef __PLAT_NGPS__

	#ifdef __NOPT_ASSERT__
	// Use the chunk of memory that was set aside for script debug stuff in NGPS.lk
	uint32 script_debug_mem_size=(uint32)(_script_end-_script_start);

	// Check there is enough space.
	uint32 hash_table_size=(1<<CHECKSUM_LOOKUP_HASH_BITS) * CHECKSUM_ENTRIES_PER_SLOT * sizeof(SChecksumName);	
	Dbg_MsgAssert(script_debug_mem_size>=hash_table_size+CHECKSUM_NAME_SPACE,(
				  "Chunk of debug memory reserved for script stuff is not big enough.\nRequired=%d, got=%d",
				  hash_table_size+CHECKSUM_NAME_SPACE, script_debug_mem_size));
	#endif				  
				  
	sp_checksum_name_hash_table=(SChecksumName*)_script_start;
	sp_checksum_names=(char*)(sp_checksum_name_hash_table+((1<<CHECKSUM_LOOKUP_HASH_BITS) * CHECKSUM_ENTRIES_PER_SLOT));
	sp_end_of_checksum_names=sp_checksum_names;

	// Set up pointers to the locations of sp_checksum_names and sp_end_of_checksum_names
	// so that later the script debugger can find the list of names using the target manager API 
	// and register their checksums too.
	Dbg_MsgAssert((uint32)_script_debugger_start==Dbg::SCRIPT_DEBUGGER_MEMORY_START,("Need to update SCRIPT_DEBUGGER_MEMORY_START in debugger.h to match the location of the script_debugger group in ngps.lk"));
	Dbg::SChecksumNameInfo *p_checksum_name_info=(Dbg::SChecksumNameInfo*)_script_debugger_start;
	p_checksum_name_info->mppStart=&sp_checksum_names;
	p_checksum_name_info->mppEnd=&sp_end_of_checksum_names;
	
	#else

	// Just allocate the arrays off the heap.
	
	// Checksum-name lookup stuff.
	sp_checksum_name_hash_table=(SChecksumName*)Mem::Malloc((1<<CHECKSUM_LOOKUP_HASH_BITS) * CHECKSUM_ENTRIES_PER_SLOT * sizeof(SChecksumName));
	sp_checksum_names=(char*)Mem::Malloc(CHECKSUM_NAME_SPACE);
	sp_end_of_checksum_names=sp_checksum_names;
	
	#endif // __PLAT_NGPS__
	
	// Initialise the hash table.
	for (int i=0; i<(1<<CHECKSUM_LOOKUP_HASH_BITS) * CHECKSUM_ENTRIES_PER_SLOT; ++i)
	{
		sp_checksum_name_hash_table[i].mChecksum=0;
		sp_checksum_name_hash_table[i].mpName=NULL;
	}
	// Initialise the pile of strings.
	*sp_checksum_names=0;
}

void DeallocateChecksumNameLookupTables()
{
	if (!Config::GotExtraMemory())
	{
		return;
	}
	
	Dbg_MsgAssert(sp_checksum_name_hash_table,("NULL sp_checksum_name_hash_table ?"));				  
	Dbg_MsgAssert(sp_checksum_names,("NULL sp_checksum_names ?"));				  
	
	#ifdef __PLAT_NGPS__
	// Nothing to delete on PS2 because the arrays were not dynamically allocated.
	#else
	Mem::Free(sp_checksum_name_hash_table);
	Mem::Free(sp_checksum_names);
	#endif
	
	sp_checksum_name_hash_table=NULL;
	sp_checksum_names=NULL;
	sp_end_of_checksum_names=NULL;
}	

// Used by the script debugger code to send the set of checksum names to the debugger program
// running on the PC.
void GetChecksumNamesBuffer(char **pp_start, char **pp_end)
{
	Dbg_MsgAssert(pp_start && pp_end,("pp_start and pp_end must be non-NULL"));
	
	if (!Config::GotExtraMemory())
	{
		*pp_start=NULL;
		*pp_end=NULL;
		return;
	}

	*pp_start=sp_checksum_names;
	*pp_end=sp_end_of_checksum_names;
}	

//uint32 sNumChecksums=0;
void AddChecksumName(uint32 checksum, const char *p_name)
{
	if (!Config::GotExtraMemory())
	{
		return;
	}
	
	Dbg_MsgAssert(checksum,("Zero checksum sent to AddChecksumName"));
	Dbg_MsgAssert(p_name,("NULL p_name sent to AddChecksumName"));
	Dbg_MsgAssert(sp_checksum_name_hash_table,("NULL sp_checksum_name_hash_table"));
	
	SChecksumName *p_first=&sp_checksum_name_hash_table[(checksum&((1<<CHECKSUM_LOOKUP_HASH_BITS)-1))*CHECKSUM_ENTRIES_PER_SLOT];
	int c=0;
	while (p_first->mpName)
	{
		// Already a name here.
		// See if it has the same checksum.
		if (p_first->mChecksum==checksum)
		{
			// It does! Check whether it is the same name.
			if (stricmp(p_first->mpName,p_name)==0)
			{
				// Phew, the name matches. No need to do anything.
				return;
			}
			else
			{
				// Oh bugger, a checksum clash.
				#ifdef __PLAT_WN32__
				#ifdef __QDEBUG__
				char p_foo[1024];
				sprintf(p_foo,"Checksum clash between %s and %s, both have checksum 0x%08x",p_name,p_first->mpName,checksum);
				MessageBox(NULL,p_foo,"Warning",MB_OK);
				#endif
				// Carry on anyway, won't cause a crash.
				#else				
				Dbg_MsgAssert(0,("Checksum clash between %s and %s, both have checksum 0x%08x",p_name,p_first->mpName,checksum));
				#endif
				return;
			}
		}
		
		// Not the same checksum. Onto the next entry.
		++c;
		Dbg_MsgAssert(c<CHECKSUM_ENTRIES_PER_SLOT,("Need to increase CHECKSUM_ENTRIES_PER_SLOT in checksum.cpp"));
		++p_first;
	}
	
	//++sNumChecksums;
	//printf("sNumChecksums=%d\n",sNumChecksums);
	
	p_first->mChecksum=checksum;
	p_first->mpName=sp_end_of_checksum_names;
	
	int space_left=CHECKSUM_NAME_SPACE-(sp_end_of_checksum_names-sp_checksum_names);
	while (*p_name)
	{
		Dbg_MsgAssert(space_left>0,("Need to increase CHECKSUM_NAME_SPACE in checksum.cpp"));
		*sp_end_of_checksum_names++=*p_name++;
		--space_left;
	}	
	Dbg_MsgAssert(space_left>0,("Need to increase CHECKSUM_NAME_SPACE in checksum.cpp"));
	*sp_end_of_checksum_names++=0;
}

static char sp_unknown_checksum_buf[100];

// Returns NULL if not found.
// This version is not safe to use in a printf because it may return NULL, but can be handy for
// when the calling code needs to do something special if the name is not found.
const char *FindChecksumNameNULL(uint32 checksum)
{
	if (!Config::GotExtraMemory())
	{
		return NULL;
	}
	
	Dbg_MsgAssert(sp_checksum_name_hash_table,("NULL sp_checksum_name_hash_table"));
	SChecksumName *p_first=&sp_checksum_name_hash_table[(checksum&((1<<CHECKSUM_LOOKUP_HASH_BITS)-1))*CHECKSUM_ENTRIES_PER_SLOT];
	int c=CHECKSUM_ENTRIES_PER_SLOT;
	while (p_first->mpName)
	{
		if (p_first->mChecksum==checksum)
		{
			return p_first->mpName;
		}	
		--c;
		if (c==0)
		{
			// Run out of slots.
			break;
		}	
		++p_first;
	}
	
	return NULL;		
}

// Returns "Unknown" if not found.
// This version is always safe to use in a printf.
const char *FindChecksumName(uint32 checksum)
{
	const char *p_name=FindChecksumNameNULL(checksum);
	if (p_name)	
	{
		return p_name;
	}
	else
	{
		#ifdef __PLAT_WN32__
		// The monitor.exe application requires that the unknown checksum be
		// displayed like this.
		sprintf(sp_unknown_checksum_buf,"0x%08x",checksum);	
		#else
		sprintf(sp_unknown_checksum_buf,"Unknown(0x%08x)",checksum);	
		#endif
		return sp_unknown_checksum_buf;		
	}	
}


} // namespace Script

