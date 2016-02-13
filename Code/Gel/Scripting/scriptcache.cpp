#include <gel/scripting/scriptcache.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/parse.h>
#include <core/compress.h>
#include <gel/mainloop.h>

#ifdef __PLAT_NGC__
#include <sys/ngc/p_aram.h>
#include <sys/ngc/p_dma.h>
#endif		// __PLAT_NGC__

#ifdef NO_SCRIPT_CACHING
#else
DefinePoolableClass(Script::CScriptCacheEntry);
#endif

// Make CScriptCache::GetDebugInfo determine how much the cache is 'thrashing' (ie, average number of new script
// entries created per frame over last second, & indicate the max)
// Make scripts not get removed from the cache straight away when their usage hits 0, but just
// remove the oldest zero-usage script when the heap is full and a new script needs to go in.
// Make CScriptCache::GetDebugInfo sort the array of scripts in order of usage

// Implement compression in qcomp.
// Test, test qbr, test size of cache required, test speed.
// Make it possible to switch back to no caching very easily. (mybe even dynamically?)
namespace Script
{

DefineSingletonClass( CScriptCache, "Script cache module" );

#ifdef NO_SCRIPT_CACHING
CScriptCache::CScriptCache()
{
}

CScriptCache::~CScriptCache()
{
}

void CScriptCache::v_start_cb ( void )
{
}

void CScriptCache::v_stop_cb ( void )
{
}

uint8 *CScriptCache::GetScript(uint32 scriptName)
{
    CSymbolTableEntry *p_entry=Resolve(scriptName);
    if (p_entry)
    {
        Dbg_MsgAssert(p_entry->mType==ESYMBOLTYPE_QSCRIPT,("Symbol %s is not a QScript",FindChecksumName(scriptName)));
        Dbg_MsgAssert(p_entry->mpScript,("NULL p_entry->mpScript"));

		return p_entry->mpScript+4; // +4 to skip over the contents checksum
	}
	
	return NULL;
}

void CScriptCache::DecrementScriptUsage(uint32 scriptName)
{
}

void CScriptCache::RefreshAfterReload(uint32 scriptName)
{
}

void CScriptCache::DeleteZeroUsageStraightAway()
{
}

void CScriptCache::DeleteOldestZeroUsageWhenNecessary()
{
}

#ifdef __NOPT_ASSERT__
void CScriptCache::GetDebugInfo( Script::CStruct* p_info )
{
	p_info->AddChecksum(NONAME,CRCD(0xf34b1528,"ScriptCachingDisabled"));
}
#endif

#else

CScriptCacheEntry::CScriptCacheEntry()
{
	mpDecompressedScript=NULL;
	mUsage=0;
	mScriptNameChecksum=0;
	mpNext=NULL;
	mpPrevious=NULL;
}

CScriptCacheEntry::~CScriptCacheEntry()
{
	// This is just to make sure nothing accidentally deletes an entry in the CScriptCache's
	// list of zero-usage entries. It won't work if there is only one entry in the list, but
	// it's better than nothing.
	Dbg_MsgAssert(mpNext==NULL,("CScriptCacheEntry::mpNext not NULL !"));
	Dbg_MsgAssert(mpPrevious==NULL,("CScriptCacheEntry::mpPrevious not NULL !"));
	
	if (mpDecompressedScript)
	{
		Mem::Free(mpDecompressedScript);
	}	
}	

CScriptCache::CScriptCache()
{
	Dbg_MsgAssert(IDEAL_MAX_DECOMPRESSED_SCRIPTS < MAX_DECOMPRESSED_SCRIPTS,("IDEAL_MAX_DECOMPRESSED_SCRIPTS should be less than MAX_DECOMPRESSED_SCRIPTS"));
	
	#ifdef __NOPT_ASSERT__
	mp_logic_task=NULL;
	m_current_decompress_count_index=0;
	for (int i=0; i<MAX_DECOMPRESS_COUNTS; ++i)
	{
		mp_decompress_counts[i]=0;
	}	
	m_num_used_scripts=0;
	m_max_used_scripts=0;
	#endif
	
	// mp_cache_hash_table is allocated here rather than in v_start_cb because v_start_cb does not
	// get called until the first frame of the game loop, which was causing a problem on XBox
	// because Nx::CEngine::sStartEngine(); needs to run a script, and that call occurs early in main.cpp
	// before the main loop.
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	mp_cache_hash_table = new Lst::HashTable<CScriptCacheEntry>(8);
	Mem::Manager::sHandle().PopContext();
	
	mp_first_zero_usage=NULL;
	mp_last_zero_usage=NULL;
	
	m_delete_zero_usage_straight_away=false;
}

CScriptCache::~CScriptCache()
{
	mp_cache_hash_table->IterateStart();
	CScriptCacheEntry *p_cache_entry = mp_cache_hash_table->IterateNext();
	while (p_cache_entry)
	{
		// Would it be better to just flush all the items after this loop instead?
		// (Ie, could flushing here confuse IterateNext ?)
		mp_cache_hash_table->FlushItem(p_cache_entry->mScriptNameChecksum);
		
		// Clear these to prevent ~CScriptCacheEntry from asserting.
		p_cache_entry->mpNext=NULL;
		p_cache_entry->mpPrevious=NULL;
		delete p_cache_entry;
		
		p_cache_entry = mp_cache_hash_table->IterateNext();
	}

	delete mp_cache_hash_table;
	
	#ifdef __NOPT_ASSERT__
	if (mp_logic_task)
	{
		delete mp_logic_task;
	}	
	#endif	
}

void CScriptCache::v_start_cb ( void )
{
	#ifdef __NOPT_ASSERT__
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	mp_logic_task = new Tsk::Task< CScriptCache > ( CScriptCache::s_logic_code, *this );

	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	mlp_manager->AddLogicTask( *mp_logic_task );
	Mem::Manager::sHandle().PopContext();
	#endif
}

void CScriptCache::v_stop_cb ( void )
{
	#ifdef __NOPT_ASSERT__
	mp_logic_task->Remove();
	#endif
}

void CScriptCache::add_to_zero_usage_list(CScriptCacheEntry *p_entry)
{
	Dbg_MsgAssert(p_entry,("NULL p_entry sent to add_to_zero_usage_list"));
	
	#ifdef __NOPT_ASSERT__
	CScriptCacheEntry *p_search=mp_first_zero_usage;
	while (p_search)
	{
		Dbg_MsgAssert(p_search != p_entry,("p_entry is already in the zero-usage list !"));
		p_search=p_search->mpNext;
	}	
	#endif
	
	p_entry->mpPrevious=NULL;
	p_entry->mpNext=mp_first_zero_usage;
	mp_first_zero_usage=p_entry;
	if (p_entry->mpNext)
	{
		p_entry->mpNext->mpPrevious=p_entry;
	}
	if (!mp_last_zero_usage)
	{
		mp_last_zero_usage=mp_first_zero_usage;
	}	
}

void CScriptCache::remove_from_zero_usage_list(CScriptCacheEntry *p_entry)
{
	Dbg_MsgAssert(p_entry,("NULL p_entry sent to remove_from_zero_usage_list"));
	
	// Unlink the entry from the list.
	if (p_entry->mpPrevious)
	{
		p_entry->mpPrevious->mpNext=p_entry->mpNext;
	}
	if (p_entry->mpNext)
	{
		p_entry->mpNext->mpPrevious=p_entry->mpPrevious;
	}
		
	// Update mp_last_zero_usage and mp_first_zero_usage
	if (p_entry==mp_first_zero_usage)
	{
		mp_first_zero_usage=p_entry->mpNext;
	}
	if (p_entry==mp_last_zero_usage)
	{
		mp_last_zero_usage=p_entry->mpPrevious;
	}
	
	p_entry->mpNext=NULL;
	p_entry->mpPrevious=NULL;
}

// This function must be used to delete a CScriptCacheEntry rather than deleting it directly,
// since this function will update the list of zero-usage entries if necessary.
void CScriptCache::delete_entry(CScriptCacheEntry *p_entry)
{
	Dbg_MsgAssert(p_entry,("NULL p_entry sent to delete_entry"));
	
	mp_cache_hash_table->FlushItem(p_entry->mScriptNameChecksum);
	// Note: If the entry is not in the zero-usage list, this function won't do anything, so safe to call.
	remove_from_zero_usage_list(p_entry);
	delete p_entry;
}

// The scriptName is only passed so that the assert can print the name of the script
// if no space could be freed up for it.
void CScriptCache::remove_some_old_scripts(int space_required, uint32 scriptName)
{
	while (true)
	{
		int largest_malloc_possible=Mem::Manager::sHandle().ScriptHeap()->LargestFreeBlock();
		largest_malloc_possible-=128; // Actually, 32 seems OK, but have a bigger margin just to be sure.
		
		if (space_required <= largest_malloc_possible)
		{
			// Hurrah! It's all clear to decompress another script since we know there is enough heap to hold it.
			
			// Try to keep the total number of decompressed scripts within IDEAL_MAX_DECOMPRESSED_SCRIPTS
			// by removing unused ones until either enough have been removed or there are no more to remove.
			
			// This way the average memory usage can be controlled by tweaking IDEAL_MAX_DECOMPRESSED_SCRIPTS
			// whilst spikes in the number of scripts are still permitted up to the limit set by
			// the pool size of MAX_DECOMPRESSED_SCRIPTS. If that limit is reached, the code will assert.
			while (CScriptCacheEntry::SGetNumUsedItems() >= IDEAL_MAX_DECOMPRESSED_SCRIPTS && mp_last_zero_usage)
			{
				delete_entry(mp_last_zero_usage);
			}	
			break;
		}	

		// Free up some space by deleting the oldest zero-usage decompressed script.
		
		if (!mp_last_zero_usage)
		{
			// Eeeek! There's nothing left to delete ...
			Dbg_MsgAssert(0,("Script heap overflow when trying to allocate decompressed script '%s' of size %d bytes!",Script::FindChecksumName(scriptName),space_required));
		}
		
		//printf("%d: Deleting %s from cache, new free space = %d\n",space_required,Script::FindChecksumName(mp_last_zero_usage->mScriptNameChecksum),Mem::Manager::sHandle().ScriptHeap()->LargestFreeBlock()-128);
		delete_entry(mp_last_zero_usage);
	}	
}

uint8 *CScriptCache::GetScript(uint32 scriptName)
{
    CSymbolTableEntry *p_entry=Resolve(scriptName);
    if (p_entry)
    {
        Dbg_MsgAssert(p_entry->mType==ESYMBOLTYPE_QSCRIPT,("Symbol %s is not a QScript",FindChecksumName(scriptName)));
#ifndef __PLAT_NGC__
        Dbg_MsgAssert(p_entry->mpScript,("NULL p_entry->mpScript"));
#endif		// __PLAT_NGC__
		
		// Change the passed scriptName to be the actual scriptName so that when decompressed it gets registered in
		// the cache under the correct name.
		// Otherwise, if a SpawnScript Foo is done after Foo has been changed using the 'change' command, it
		// will spawn the old script again, since it will be in the cache under Foo.
		scriptName=p_entry->mNameChecksum;
	}	


	CScriptCacheEntry *p_cache_entry=mp_cache_hash_table->GetItem(scriptName);
	if (p_cache_entry)
	{
		++p_cache_entry->mUsage;
		
		#ifdef __NOPT_ASSERT__
		if (p_cache_entry->mUsage==1)
		{
			++m_num_used_scripts;
			if (m_num_used_scripts > m_max_used_scripts)
			{
				m_max_used_scripts=m_num_used_scripts;
			}	
		}	
		#endif
		
		remove_from_zero_usage_list(p_cache_entry);
		return p_cache_entry->mpDecompressedScript;
	}	
		
    if (p_entry)
    {
#ifdef __PLAT_NGC__
		enum
		{
			COMPRESS_BUFFER_SIZE=20000,
		};	
		uint8 p_compress_buffer[COMPRESS_BUFFER_SIZE];
		uint32 header[(SCRIPT_HEADER_SIZE/4)];
		NsDMA::toMRAM( header, p_entry->mScriptOffset, SCRIPT_HEADER_SIZE );
		uint32 uncompressed_size		= header[1];
		uint32 compressed_size			= header[2];
		NsDMA::toMRAM( p_compress_buffer, p_entry->mScriptOffset + SCRIPT_HEADER_SIZE, compressed_size );
#else
		// Note: The script contents checksum is at offset 0, but it is not needed.
		uint32 uncompressed_size		= *(uint32*)(p_entry->mpScript+4);
		uint32 compressed_size			= *(uint32*)(p_entry->mpScript+8);
#endif		// __PLAT_NGC__
		
		remove_some_old_scripts(uncompressed_size,scriptName);

		CScriptCacheEntry *p_cache_entry=new CScriptCacheEntry;
		
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
		uint8 *p_new_script=(uint8*)Mem::Malloc(uncompressed_size);
		Mem::Manager::sHandle().PopContext();
		
		if (uncompressed_size > compressed_size)
		{
			#ifdef	__NOPT_ASSERT__		
			uint8 *p_end=
			#endif
#ifdef __PLAT_NGC__
				DecodeLZSS(p_compress_buffer,p_new_script,compressed_size);
#else
				DecodeLZSS(p_entry->mpScript+SCRIPT_HEADER_SIZE,p_new_script,compressed_size);
#endif		// __PLAT_NGC__
			Dbg_MsgAssert(p_end==p_new_script+uncompressed_size,("Eh? p_end is not right?"));
		}
		else
		{
			// The script is uncompressed so just copy it over. Saves, errr, 1K altogether, oh well.
			Dbg_MsgAssert(uncompressed_size == compressed_size,("Expected uncompressed_size==compressed_size"));
#ifdef __PLAT_NGC__

			uint8 *p_source=p_compress_buffer;
#else
			uint8 *p_source=p_entry->mpScript+SCRIPT_HEADER_SIZE;
#endif		// __PLAT_NGC__
			uint8 *p_dest=p_new_script;
			for (uint32 i=0; i<uncompressed_size; ++i)
			{
				*p_dest++=*p_source++;
			}	
		}
		
		PreProcessScript(p_new_script);
		
		p_cache_entry->mpDecompressedScript=p_new_script;
		p_cache_entry->mUsage=1;
		p_cache_entry->mScriptNameChecksum=scriptName;
		
		#ifdef __NOPT_ASSERT__
		++m_num_used_scripts;
		if (m_num_used_scripts > m_max_used_scripts)
		{
			m_max_used_scripts=m_num_used_scripts;
		}	
		#endif
		
		
		mp_cache_hash_table->PutItem(scriptName,p_cache_entry);

		#ifdef __NOPT_ASSERT__
		++mp_decompress_counts[m_current_decompress_count_index];
		#endif
		
		return p_cache_entry->mpDecompressedScript;
	}
	
	return NULL;
}

void CScriptCache::DecrementScriptUsage(uint32 scriptName)
{
	CScriptCacheEntry *p_cache_entry=mp_cache_hash_table->GetItem(scriptName);
	if (p_cache_entry)
	{
		Dbg_MsgAssert(p_cache_entry->mUsage,("Zero cache entry usage for script '%s'",Script::FindChecksumName(scriptName)));
		
		--p_cache_entry->mUsage;
		if (p_cache_entry->mUsage==0)
		{
			#ifdef __NOPT_ASSERT__
			--m_num_used_scripts;
			#endif
			
			if (m_delete_zero_usage_straight_away)
			{
				// For testing
				delete_entry(p_cache_entry);
			}
			else
			{
				add_to_zero_usage_list(p_cache_entry);
			}	
		}
	}
}

// Needed for when a script is qbr'd.
void CScriptCache::RefreshAfterReload(uint32 scriptName)
{
	CScriptCacheEntry *p_cache_entry=mp_cache_hash_table->GetItem(scriptName);
	if (p_cache_entry)
	{
		// Store the old usage
		uint32 old_usage=p_cache_entry->mUsage;
		
		// Remove from the cache, then add it again so that it is up to date.
		delete_entry(p_cache_entry);
		GetScript(scriptName);
		
		// Put the usage value back.
		p_cache_entry=mp_cache_hash_table->GetItem(scriptName);
		Dbg_MsgAssert(p_cache_entry,("NULL p_cache_entry ?"));
		p_cache_entry->mUsage=old_usage;
	}	
}

void CScriptCache::DeleteZeroUsageStraightAway()
{
	m_delete_zero_usage_straight_away=true;
}

void CScriptCache::DeleteOldestZeroUsageWhenNecessary()
{
	m_delete_zero_usage_straight_away=false;
}

// Scans through the currently decompressed scripts looking to see
// which one p_token points into, and returns the name of the script, or Unknown
// if it can;t find it.
const char *CScriptCache::GetSourceFile(uint8 *p_token)
{
	mp_cache_hash_table->IterateStart();
	CScriptCacheEntry *p_cache_entry = mp_cache_hash_table->IterateNext();
	while (p_cache_entry)
	{
		if (PointsIntoScript(p_token,p_cache_entry->mpDecompressedScript))
		{
		    CSymbolTableEntry *p_entry=Resolve(p_cache_entry->mScriptNameChecksum);
			Dbg_MsgAssert(p_entry,("Eh? Decompressed script has no source symbol table entry ?"));
			return Script::FindChecksumName(p_entry->mSourceFileNameChecksum);
		}
		
		p_cache_entry = mp_cache_hash_table->IterateNext();		
	}
	
	return "Unknown";
}

#ifdef __NOPT_ASSERT__
void CScriptCache::s_logic_code ( const Tsk::Task< CScriptCache >& task )
{
	CScriptCache&	mdl = task.GetData();
	Dbg_AssertType ( &task, Tsk::Task< CScriptCache > );

	++mdl.m_current_decompress_count_index;
	if (mdl.m_current_decompress_count_index >= MAX_DECOMPRESS_COUNTS)
	{
		mdl.m_current_decompress_count_index=0;
	}	
	mdl.mp_decompress_counts[mdl.m_current_decompress_count_index]=0;
}

void CScriptCache::GetDebugInfo( Script::CStruct* p_info )
{
	Script::CStruct *p_script_cache_info=new Script::CStruct;
	
	p_script_cache_info->AddInteger(CRCD(0xe8081c20,"NumUsedScripts"),m_num_used_scripts);
	p_script_cache_info->AddInteger(CRCD(0x746c6f5b,"MaxUsedScripts"),m_max_used_scripts);
	
	int max=0;
	int total=0;
	for (int i=0; i<MAX_DECOMPRESS_COUNTS; ++i)
	{
		if (mp_decompress_counts[i] > max)
		{
			max=mp_decompress_counts[i];
		}
		total+=mp_decompress_counts[i];
	}	
	p_script_cache_info->AddInteger(CRCD(0xc5eb1eb2,"MaxDecompressCount"),max);
	p_script_cache_info->AddInteger(CRCD(0xdd84ffd8,"AverageDecompressCount"),total/MAX_DECOMPRESS_COUNTS);
	
	mp_cache_hash_table->IterateStart();
	int num_scripts_in_cache=0;
	CScriptCacheEntry *p_cache_entry = mp_cache_hash_table->IterateNext();
	while (p_cache_entry)
	{
		++num_scripts_in_cache;
		p_cache_entry = mp_cache_hash_table->IterateNext();		
	}

	p_script_cache_info->AddInteger(CRCD(0xb6d1536,"NumScriptsInCache"),num_scripts_in_cache);

	if (num_scripts_in_cache)
	{
		Script::CArray *p_scripts_array=new Script::CArray;
		p_scripts_array->SetSizeAndType(num_scripts_in_cache,ESYMBOLTYPE_STRUCTURE);
		
		int alloc_total=0;
		mp_cache_hash_table->IterateStart();
		int array_index=0;
		p_cache_entry = mp_cache_hash_table->IterateNext();
		while (p_cache_entry)
		{
			Script::CStruct *p_cache_entry_info=new Script::CStruct;
			p_cache_entry_info->AddChecksum(CRCD(0xe37e78c5,"Script"),p_cache_entry->mScriptNameChecksum);
			p_cache_entry_info->AddInteger(CRCD(0x2f14a18f,"Usage"),p_cache_entry->mUsage);
			
			int alloc_size=Mem::GetAllocSize(p_cache_entry->mpDecompressedScript);
			p_cache_entry_info->AddInteger(CRCD(0x454a3b10,"ScriptSize"),alloc_size);
			alloc_total+=alloc_size;
			
			p_scripts_array->SetStructure(array_index,p_cache_entry_info);
			p_cache_entry = mp_cache_hash_table->IterateNext();		
			++array_index;
		}
		
		p_script_cache_info->AddInteger(CRCD(0x43dde78c,"TotalScriptSizes"),alloc_total);
		p_script_cache_info->AddArrayPointer(CRCD(0xf964b9dd,"CachedScripts"),p_scripts_array);
	}
		
	p_info->AddStructurePointer(CRCD(0x3d2bde05,"ScriptCacheInfo"),p_script_cache_info);
}
#endif // #ifdef __NOPT_ASSERT__

#endif // #ifdef NO_SCRIPT_CACHING

} // namespace Script

