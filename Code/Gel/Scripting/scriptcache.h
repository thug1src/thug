#ifndef	__SCRIPTING_SCRIPTCACHE_H
#define	__SCRIPTING_SCRIPTCACHE_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

#ifndef __CORE_LIST_HASHTABLE_H
#include <core/hashtable.h>
#endif

#ifndef __GEL_MODULE_H
#include <gel/module.h>
#endif

// If this is defined then GetScript will just return the mpScript as stored in the
// CSymbolTableEntry.
#ifdef __PLAT_WN32__
// No caching when compiled on PC since it is not needed and would require linking in memory manager stuff.
#define NO_SCRIPT_CACHING
#endif
//#define NO_SCRIPT_CACHING

// The total size of the CScriptCacheEntry pool.
// This is the max number of decompressed scripts that can exist at once.
// The code will assert if this number of scripts is exceeded.
// This may be set to be quite large since the CScriptCacheEntry is quite small, 24 bytes each.
// It needs to be fairly large to allow for spikes in the number of scripts executing (see TT1417)
#define MAX_DECOMPRESSED_SCRIPTS 300

// The code will attempt to keep the total number of decompressed scripts to no more than this value
// by deleting unused scripts.
// This allows us to control the average number of decompressed scripts whilst still allowing spikes
// up to MAX_DECOMPRESSED_SCRIPTS without the code asserting.
// Allowing spikes is necessary because sometimes a lot of scripts may exist which don't use up much
// script heap altogether.

// The larger IDEAL_MAX_DECOMPRESSED_SCRIPTS is, the less often the code will have to
// decompress scripts during gameplay, hence less cpu time will be used.
// However, the bigger it is, the more script heap will be used up storing decompressed scripts.
#define IDEAL_MAX_DECOMPRESSED_SCRIPTS 100

namespace Script
{
class CStruct;

#ifdef NO_SCRIPT_CACHING
#else
class CScriptCacheEntry : public Mem::CPoolable<CScriptCacheEntry>
{
public:
	CScriptCacheEntry();
	~CScriptCacheEntry();

	// The script name is stored here so that the make_enough_space function
	// is able to flush entries from the hash table, given only a CScriptCacheEntry pointer.
	// (Entries can only be flushed from the hash table if their key is known)
	uint32 mScriptNameChecksum;
	
	uint8 *mpDecompressedScript;
	uint32 mUsage;
	
	CScriptCacheEntry *mpNext;
	CScriptCacheEntry *mpPrevious;
};
#endif // #ifdef NO_SCRIPT_CACHING

#ifdef NO_SCRIPT_CACHING
class CScriptCache   // No deriving it from Mdl::Module if not usieng it, as that foces uncessary linkages
#else
class CScriptCache  : public Mdl::Module
#endif
{
	DeclareSingletonClass( CScriptCache );
	
	void v_start_cb ( void );
	void v_stop_cb ( void );

	#ifdef NO_SCRIPT_CACHING
	#else

	void add_to_zero_usage_list(CScriptCacheEntry *p_entry);
	void remove_from_zero_usage_list(CScriptCacheEntry *p_entry);
	void delete_entry(CScriptCacheEntry *p_entry);
	void remove_some_old_scripts(int space_required, uint32 scriptName);

	#ifdef __NOPT_ASSERT__
	static Tsk::Task< CScriptCache >::Code s_logic_code;       
	Tsk::Task< CScriptCache > *mp_logic_task;
	
	enum
	{
		MAX_DECOMPRESS_COUNTS=60
	};	
	int mp_decompress_counts[MAX_DECOMPRESS_COUNTS];
	int m_current_decompress_count_index;
	
	int m_num_used_scripts; // A count of how many script entries have a usage > 0
	int m_max_used_scripts; // The max value that the above reached, used for choosing a suitable MAX_DECOMPRESSED_SCRIPTS
	#endif
	
	Lst::HashTable<CScriptCacheEntry> *mp_cache_hash_table;
	
	// Points to the first (youngest) of the zero-usage entries.
	CScriptCacheEntry *mp_first_zero_usage;
	// Points to the last (oldest) of the zero-usage entries.
	CScriptCacheEntry *mp_last_zero_usage;
	
	bool m_delete_zero_usage_straight_away;

	#endif // #ifdef NO_SCRIPT_CACHING
public:
	CScriptCache();
	virtual	~CScriptCache();

	uint8 *GetScript(uint32 scriptName);
	void DecrementScriptUsage(uint32 scriptName);
	void RefreshAfterReload(uint32 scriptName);

	void DeleteZeroUsageStraightAway();
	void DeleteOldestZeroUsageWhenNecessary();

	#ifdef NO_SCRIPT_CACHING
	#else
	const char *GetSourceFile(uint8 *p_token);
	#endif
		
	#ifdef __NOPT_ASSERT__
	void GetDebugInfo( Script::CStruct* p_info );
	#endif
};
	
} // namespace Script

#endif // #ifndef	__SCRIPTING_SCRIPTCACHE_H
