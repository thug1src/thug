///////////////////////////////////////////////////////////////////////////////////////
//
// init.cpp		KSH 19 Oct 2001
//
// Functions for initialising various script stuff.
// 
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/init.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/component.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/string.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/script.h>
#include <gel/scripting/parse.h>
#include <gel/scripting/scriptcache.h>
#include <core/crc.h>

namespace Script
{

// MEMOPT This is where all the pools of script things get allocated.
void AllocatePools()
{
	// Sets up the checksum name-lookup hash table.
	// On the PS2 this does not actually do any dynamic allocation, but uses the space set aside
	// in NGPS.lk and defined by _script_start and _script_end
	// On other platforms it does dynamically allocate the space.
	// The function will not do anything if Config::GotExtraMemory() returns false.
	Mem::PushMemProfile("AllocateChecksumNameLookupTables");
	AllocateChecksumNameLookupTables();
	Mem::PopMemProfile();
	
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());

	#ifndef __PLAT_WN32__
	
	#ifdef NO_SCRIPT_CACHING
	#else
	Mem::PushMemProfile("CScriptCacheEntry");
	// 32ish bytes each
	CScriptCacheEntry::SCreatePool(MAX_DECOMPRESSED_SCRIPTS, "CScriptCacheEntry");
	Mem::PopMemProfile();
	#endif
		
	Mem::PushMemProfile("CComponent and Reserve CComponent");
	
	// 16 bytes each
	CComponent::SCreatePool(84000, "CComponent");  // Mick:  increased by 2000 (82000 to 84000) to account for gap lists

	Mem::PopMemProfile();
	

	Mem::PushMemProfile("CStruct");
	// 4 bytes each  (Actually 8, or 12 with asserts)
	CStruct::SCreatePool(18800, "CStruct");
	Mem::PopMemProfile();
	
	// 12 bytes each  (Actually 16)
	Mem::PushMemProfile("CVector");
	CVector::SCreatePool(9600, "CVector");
	Mem::PopMemProfile();
	
	// 8 bytes each	  (Actually 12)
	Mem::PushMemProfile("CPair");
	CPair::SCreatePool(1000, "CPair");	 // Mick: was 5000, but only used 308 at last count
	Mem::PopMemProfile();
	
	// 12 bytes each
	Mem::PushMemProfile("CArray");
	//CArray::SCreatePool(6000, "CArray");
	CArray::SCreatePool(7500, "CArray");
	Mem::PopMemProfile();
	
	
	// 12 bytes each (actually 16)
	Mem::PushMemProfile("CSymbolTableEntry");
	CSymbolTableEntry::SCreatePool(8500, "CSymbolTableEntry");
	Mem::PopMemProfile();

	Mem::PushMemProfile("CScript");
	// 1080 bytes each, bloody hell (Actually ~1238!!!)
	CScript::SCreatePool(MAX_CSCRIPTS, "CScript");
	Mem::PopMemProfile();

	Mem::PushMemProfile("CStoredRandom");
	// 80 bytes each (100)
	CStoredRandom::SCreatePool(MAX_STORED_RANDOMS,"CStoredRandom");
	Mem::PopMemProfile();
	#endif


	// This will create a further 4096 CSymbolTableEntry's, but as a contiguous array.
//	Mem::PushMemProfile("CreateSymbolHashTable");
//	CreateSymbolHashTable();
//	Mem::PopMemProfile();
	
	// 60500 bytes was needed for foreign languages on THPS3, 48000 for English.
	// 4000 is the max number of strings.
	//AllocatePermanentStringHeap(60500,4000);
	Mem::PushMemProfile("AllocatePermanentStringHeap");
	AllocatePermanentStringHeap(160000,7000);	
	Mem::PopMemProfile();
	
	Mem::Manager::sHandle().PopContext();
}

void DeallocatePools()
{
	DeallocatePermanentStringHeap();
	DestroySymbolHashTable();
	
	#ifndef __PLAT_WN32__
	CScript::SRemovePool();
	CSymbolTableEntry::SRemovePool();
	CArray::SRemovePool();
	CStruct::SRemovePool();
	CPair::SRemovePool();
	CVector::SRemovePool();
	CComponent::SRemovePool();
	#endif
		
	DeallocateChecksumNameLookupTables();
}

// Adds all the c-functions listed in the passed array to the symbol table.
void RegisterCFunctions(SCFunction *p_cFunctions, uint32 numFunctions)
{
	Dbg_MsgAssert(p_cFunctions,("NULL p_cFunctions"));
	
	for (uint32 i=0; i<numFunctions; ++i)
	{
		uint32 name_checksum=Crc::GenerateCRCFromString(p_cFunctions[i].mpName);

		AddChecksumName(name_checksum,p_cFunctions[i].mpName);
		
		// Check that there is no symbol already with this name.
		#ifdef __NOPT_ASSERT__
		CSymbolTableEntry *p_existing_entry=LookUpSymbol(name_checksum);
		Dbg_MsgAssert(p_existing_entry==NULL,(
					  "The C-function named '%s' is already defined in the file %s where it has type '%s'",
					  p_cFunctions[i].mpName,
					  p_existing_entry->mType==ESYMBOLTYPE_CFUNCTION || p_existing_entry->mType==ESYMBOLTYPE_MEMBERFUNCTION ? "ftables.cpp":FindChecksumName(p_existing_entry->mSourceFileNameChecksum),
					  GetTypeName(p_existing_entry->mType)));
		#endif		
		
		CSymbolTableEntry *p_new=CreateNewSymbolEntry(name_checksum);
		Dbg_MsgAssert(p_new,("NULL p_new ??"));
	
		p_new->mType=ESYMBOLTYPE_CFUNCTION;
		p_new->mpCFunction=p_cFunctions[i].mpFunction;
		// Note: Not setting p_new->mSourceFileNameChecksum to anything, just leaving it as 0, since there is no
		// source qb file.
	}
}

// Adds all the member functions listed in the passed array to the symbol table.
void RegisterMemberFunctions(const char **pp_memberFunctions, uint32 numFunctions)
{
	Dbg_MsgAssert(pp_memberFunctions,("NULL pp_memberFunctions"));
	
	for (uint32 i=0; i<numFunctions; ++i)
	{
		uint32 name_checksum=Crc::GenerateCRCFromString(pp_memberFunctions[i]);
		AddChecksumName(name_checksum,pp_memberFunctions[i]);
		
		// Check that there is no symbol already with this name.
		#ifdef __NOPT_ASSERT__
		CSymbolTableEntry *p_existing_entry=LookUpSymbol(name_checksum);
		Dbg_MsgAssert(p_existing_entry==NULL,(
					  "The member function named '%s' is already defined in the file %s where it has type '%s'",
					  pp_memberFunctions[i],
					  p_existing_entry->mType==ESYMBOLTYPE_CFUNCTION || p_existing_entry->mType==ESYMBOLTYPE_MEMBERFUNCTION ? "ftables.cpp":FindChecksumName(p_existing_entry->mSourceFileNameChecksum),
					  GetTypeName(p_existing_entry->mType)));
		#endif
		
		CSymbolTableEntry *p_new=CreateNewSymbolEntry(name_checksum);
		Dbg_MsgAssert(p_new,("NULL p_new ??"));
	
		p_new->mType=ESYMBOLTYPE_MEMBERFUNCTION;
		// No need to set the value to anything, since the name of the symbol specifies the member function.
		
		// Note: Not setting p_new->mSourceFileNameChecksum to anything, just leaving it as 0, since there is no
		// source qb file.
	}
}

} // namespace Script
