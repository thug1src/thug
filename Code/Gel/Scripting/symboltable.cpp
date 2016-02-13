///////////////////////////////////////////////////////////////////////////////////////
//
// symboltable.cpp		KSH 17 Oct 2001
//
// Symbol hash table stuff.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <core/crc.h>

#include <sys/timer.h>

DefinePoolableClass(Script::CSymbolTableEntry);

namespace Script
{

CSymbolTableEntry::CSymbolTableEntry()
{
	// Initialise everything. CSymbolTableEntry is not derived from CClass so we don't get
	// the auro-zeroing.
	Reset();
}

// The destructor does not delete anything to avoid cyclic dependencies, so just assert
// if it look like there could be a non-NULL pointer left.
#ifdef __NOPT_ASSERT__
CSymbolTableEntry::~CSymbolTableEntry()
{
	Dbg_MsgAssert(mUnion==0,("CSymbolTableEntry still contains data, possibly an undeleted pointer"));
}
#endif

void CSymbolTableEntry::Reset()
{
    mNameChecksum=NO_NAME;
	mGotReloaded=false;
    mUsed=false;
    mType=ESYMBOLTYPE_NONE;
	mUnion=0;
    mpNext=NULL;
	mSourceFileNameChecksum=NO_NAME;
	#ifdef COUNT_USAGE
	mUsage=0;
	#endif
}

static CSymbolTableEntry *sp_hash_table=NULL;

void CreateSymbolHashTable()
{
	Dbg_MsgAssert(sp_hash_table==NULL,("sp_hash_table not NULL ?"));
	sp_hash_table=new CSymbolTableEntry[1<<NUM_HASH_BITS];
}

void DestroySymbolHashTable()
{
	Dbg_MsgAssert(sp_hash_table!=NULL,("sp_hash_table is NULL ?"));
	delete[] sp_hash_table;
	sp_hash_table=NULL;
}

// Searches for the symbol with the passed Checksum.
// If not found, it returns NULL.
/*int LookUpSymbolCount=0;
int ScriptCount=0;
int CFuncCount=0;
int MemberFuncCount=0;
int FloatCount=0;
int IntegerCount=0;
int StructureCount=0;
int ArrayCount=0;
int NothingCount=0;*/

uint32 LastVBlanks=0;

CSymbolTableEntry *LookUpSymbol(uint32 checksum)
{
	/*
	++LookUpSymbolCount;
	if (LastVBlanks!=Tmr::GetVblanks())
	{
		LastVBlanks=Tmr::GetVblanks();
		printf("%d Script:%d CFunc:%d Member Func:%d Float:%d Int:%d Struc:%d Array:%d Nothing:%d\n",LookUpSymbolCount,ScriptCount,CFuncCount,MemberFuncCount,FloatCount,IntegerCount,StructureCount,ArrayCount,NothingCount);
		LookUpSymbolCount=0;
		ScriptCount=0;
		CFuncCount=0;
		MemberFuncCount=0;
		FloatCount=0;
		IntegerCount=0;
		StructureCount=0;
		ArrayCount=0;
		NothingCount=0;
	}
	*/
	
#ifdef __PLAT_WN32__
	// GJ:  CreateSymbolHashTable was formerly called as part of the
	// script system initialization, but it was moved earlier to fix some
	// music bug (see main.cpp), which breaks those Win32 tools that
	// didn't already call CreateSymbolHashTable() explicitly...
	Dbg_MsgAssert( sp_hash_table, ( "No symbol table...  was CreateSymbolHashTable() called?" ) );
#endif

	// Instead of asserting if sp_hash_table is NULL, just return NULL.
	// This is because when the sio_manager is created in main.cpp it calls GetInteger, but this is before
	// SkateScript::Init has been called.
	// I didn't want to fiddle with the init order there in case it broke something else, so just made this
	// not assert instead.
	// MICK:  Removed that problem, so don't need this test any more.
//	if (!sp_hash_table)
//	{
//		//++NothingCount;
//		return NULL;
//	}	
	
    // Get the first entry in the list for this checksum.
    CSymbolTableEntry *p_sym=&sp_hash_table[ checksum & ((1<<NUM_HASH_BITS)-1) ];

    // If nothing here, return NULL.
	// MICK: Don't need to do this anyway, as generally the symbol WILL exists (99.9% of the time)
	// so we can afford to be slightly slower when it does not.
//    if (!p_sym->mUsed) 
//	{
//		//++NothingCount;
//		return NULL;
//	}	

#if 0	
	//MCozzini 4-30-03 optimization to avoid the p_sym test
	if(p_sym->mNameChecksum==checksum)
	{
		return p_sym;
	}
	p_sym=p_sym->mpNext;


    // Scan through the linked list until the complete checksum is found.
    while (p_sym && p_sym->mNameChecksum!=checksum) 
	{
		p_sym=p_sym->mpNext;
	}	
#else
	// it it's an unused slot, then both mNameChecksum and mpNext will be 0 (NULL)
	// so it will just drop through this test
	do 
	{
		if(p_sym->mNameChecksum == checksum)
		{
			return p_sym;	// Quicker to return here than to break
		}
		p_sym=p_sym->mpNext;
	} while (p_sym && p_sym->mNameChecksum!=checksum);
	
	// Here p_sym will be NULL	
	return p_sym;	

#endif
	
	// p_sym will be NULL if the checksum was not found.
	/*
	if (p_sym)
	{
		if (p_sym->mType==ESYMBOLTYPE_QSCRIPT)
		{
			++ScriptCount;
		}
		else if (p_sym->mType==ESYMBOLTYPE_CFUNCTION)
		{
			++CFuncCount;
		}
		else if (p_sym->mType==ESYMBOLTYPE_MEMBERFUNCTION)
		{
			++MemberFuncCount;
		}
		else if (p_sym->mType==ESYMBOLTYPE_FLOAT)
		{
			++FloatCount;
		}
		else if (p_sym->mType==ESYMBOLTYPE_INTEGER)
		{
			++IntegerCount;
		}
		else if (p_sym->mType==ESYMBOLTYPE_STRUCTURE)
		{
			++StructureCount;
		}
		else if (p_sym->mType==ESYMBOLTYPE_ARRAY)
		{
			++ArrayCount;
		}
		
	}
	else
	{
		++NothingCount;
	}*/
    return p_sym;
}

CSymbolTableEntry *LookUpSymbol(const char *p_name)
{
	return LookUpSymbol(Crc::GenerateCRCFromString(p_name));
}	

// Resolves a checksum into a symbol table entry. Does any stepping over of chains of symbols linked by
// name. Eg, Resolving a where a=b, b=c, c=d, d=6 will return the symbol d, with value 6.
// Returns NULL if the checksum cannot be resolved.
CSymbolTableEntry *Resolve(uint32 checksum)
{
    // Look up the checksum.
    CSymbolTableEntry *p_entry=LookUpSymbol(checksum);
	if (p_entry && p_entry->mType==ESYMBOLTYPE_NAME)
	{
		// If the symbol is of type name, then it may be that the name is the name of another symbol,
		// in which case we need to find that.
		// Eg we could be trying to resolve a, where a=b, b=c, c=7, in which case we need to find 7.
		// So loop until either a non-name symbol is reached, or no further symbol is reached.
		
		#ifdef __NOPT_ASSERT__ 
		// An infinite loop could occur, for example if we have a global a=b where b=a
		// This counter will make an assert go off if we try and loop a suspicious number of times.
		uint32 num_loops=0;
		#endif
	
		while (true)
		{
			CSymbolTableEntry *p_new=LookUpSymbol(p_entry->mChecksum);
			if (!p_new)
			{
				// Reached the end of the chain, so return the last entry, which will have type name.
				return p_entry;
			}
				
			if (p_new->mType!=ESYMBOLTYPE_NAME)
			{
				// Reached something not of type name, so return that.
				return p_new;
			}
			
			// Repeat.	
			p_entry=p_new;
			Dbg_MsgAssert(num_loops++<10,("Possible infinite recursion of the Resolve function when trying to resolve '%s', bailing out!",FindChecksumName(checksum)));
		}
	}
			
    return p_entry;
}

CSymbolTableEntry *Resolve(const char *p_name)
{
	return Resolve(Crc::GenerateCRCFromString(p_name));
}	

// Removes the symbol from the table.
// Used when reloading a .qb file. 
// Note: Will not delete any entities referred to by the symbol, and will assert if any of the
// pointers are not NULL.
// This is to avoid a cyclic dependency between symboltable and struct.
// It is up to the caller to have deleted these before calling this.
void RemoveSymbol(CSymbolTableEntry *p_sym)
{
	Dbg_MsgAssert(p_sym,("NULL p_sym"));
	Dbg_MsgAssert(p_sym->mUnion==0,("CSymbolTableEntry still contains data, possibly an undeleted pointer. Type='%s'",GetTypeName(p_sym->mType)));
	Dbg_MsgAssert(p_sym->mUsed,("Tried to call RemoveSymbol on an unused CSymbolTableEntry"));
	Dbg_MsgAssert(sp_hash_table!=NULL,("sp_hash_table is NULL ?"));

	// Get the head pointer of the list that p_sym is in (or should be in) 
    CSymbolTableEntry *p_head=&sp_hash_table[ p_sym->mNameChecksum & ((1<<NUM_HASH_BITS)-1) ];

	if (p_sym==p_head)
    {
        // p_sym is in the hash table itself, so it must not be deleted because it is an element of
		// a contiguous array, not allocated off the pool.

        // Check if there are further entries in the list. These will have been allocated off the pool.
        if (p_sym->mpNext)
        {
			// There are later entries in the list, so copy the first of these down
			// to fill the hole.
			CSymbolTableEntry *p_source=p_sym->mpNext;
			
			// Copy the contents. Not using the assignement operator because that won't link.
			// It is declared but not defined so as to cause it not to link, to prevent any problems
			// with copying pointers if it gets used anywhere else.
			uint32 *p_source_word=(uint32*)p_source;
			uint32 *p_dest_word=(uint32*)p_sym;
			Dbg_MsgAssert((sizeof(CSymbolTableEntry)&3)==0,("sizeof(CSymbolTableEntry) not a multiple of 4 ?"));
			for (uint32 i=0; i<sizeof(CSymbolTableEntry)/4; ++i)
			{
				*p_dest_word++=*p_source_word++;
			}	
			
			// Need to zero the union so that the destructor does not think there is an undeleted
			// pointer in it, which will make it assert.
			p_source->mUnion=0;  
			delete p_source;
        }
		else
		{
			// Just reset the contents of p_sym.
			p_sym->Reset();
		}
    }
    else
    {
        // p_sym is not in the hash table, so it must have been allocated off the pool.
		// In order to unlink it from the list we need to find the previous entry in the list.
		// So search through the list until p_sym is found.		
				
		// Start with mpNext, cos we know p_sym is not p_head		
		CSymbolTableEntry *p_previous=p_head;
		CSymbolTableEntry *p_search=p_head->mpNext;
		while (p_search)
		{
			if (p_search==p_sym)
			{
				break;
			}	
			p_previous=p_search;
			p_search=p_search->mpNext;
		}
		
		Dbg_MsgAssert(p_search,("p_sym not found in list ? (checksum='%s')",FindChecksumName(p_sym->mNameChecksum)));
		
		// Unlink p_sym and delete it.
		p_previous->mpNext=p_sym->mpNext;
		delete p_sym;
	}	
}

// Adds a new checksum to the directory and returns a pointer to the new entry.
CSymbolTableEntry *CreateNewSymbolEntry(uint32 checksum)
{
	#ifdef __NOPT_ASSERT__
	CSymbolTableEntry *p_entry=LookUpSymbol(checksum);
    Dbg_MsgAssert(p_entry==NULL,("Symbol '%s' defined twice.",FindChecksumName(checksum)));
	#endif
    
    // Get the head pointer of the list where the new symbol needs to go.
	Dbg_MsgAssert(sp_hash_table!=NULL,("sp_hash_table is NULL ?"));
    CSymbolTableEntry *p_sym=&sp_hash_table[ checksum & ((1<<NUM_HASH_BITS)-1) ];

    // If nothing in here, use this one.
    if (!p_sym->mUsed)
    {
		p_sym->Reset();
        // Set the checksum.
        p_sym->mNameChecksum=checksum;
        // Flag it as used.
        p_sym->mUsed=true;
		// Set the mGotReloaded flag so that other game-specific code can see 
		// that the symbol has changed in value.
		p_sym->mGotReloaded=true;
        return p_sym;
    }

    // Find the last element of the linked list, checking for any checksum
    // clashes on the way.
    CSymbolTableEntry *p_previous=NULL;
    while (p_sym)
    {
        Dbg_MsgAssert(p_sym->mNameChecksum!=checksum,("Checksum clash in symbol table: '%s'",FindChecksumName(checksum)));
        p_previous=p_sym;
        p_sym=p_sym->mpNext;
    }

	// Get a new entry from the pool.
    CSymbolTableEntry *p_new=new CSymbolTableEntry;
	
    // Set the checksum & next pointer.
    p_new->mNameChecksum=checksum;
    // Flag it as used.
    p_new->mUsed=true;
	// Set the mGotReloaded flag so that other game-specific code can see 
	// that the symbol has changed in value.
	p_new->mGotReloaded=true;
	
    p_new->mpNext=NULL;

    // This assert should never go off, but do it just in case.
    Dbg_MsgAssert(p_previous,("NULL p_previous ???"));
    // Stick pNew on the end of the list.
    p_previous->mpNext=p_new;

    return p_new;
}

// This function provides an easy way to loop through all the symbols in the symbol table by
// hiding all the messy logic for stepping through the hash table.
//
// If passed NULL, it will return the first entry in the symbol table (or NULL if there is nothing in it)
// If passed a pointer to a CSymbolTableEntry, it will return the next one, or NULL if none left.
//
// So if you want to loop through all the symbols just do something like:
// CSymbolTableEntry *p_sym=GetNextSymbolTableEntry(); // No need to pass NULL, it defaults to it.
// while (p_sym)
// {
// 		// ...
//		p_sym=GetNextSymbolTableEntry(p_sym);
// }
//
CSymbolTableEntry *GetNextSymbolTableEntry(CSymbolTableEntry *p_sym)
{
	// static's for keeping track of where we are in the hash table.
	static uint32 s_current_hash_index=0;
	static CSymbolTableEntry *sp_last=NULL;
	
	if (p_sym==NULL)
	{
		// They want the first symbol.
		s_current_hash_index=0;
		p_sym=sp_hash_table;
		
		// Step through the hash table entries until a used one is found.
		while (!p_sym->mUsed)
		{
			++s_current_hash_index;
			++p_sym;
			
			if (s_current_hash_index>=(1<<NUM_HASH_BITS))
			{
				// Run out of hash table.
				s_current_hash_index=0;
				sp_last=NULL;
				return NULL;
			}	
		}	
		
		sp_last=p_sym;
		return p_sym;
	}
	else
	{
		// Check that p_sym is the same as the last one returned by this function.
		
		// Currently this function assumes that the user is going to be calling it on
		// consecutive elements, such as in the example loop given in the comment for this function.
		// In theory it could be made to work when passed an arbitrary member of the symbol table, but this
		// would require it to search forward from the start each time in order to find out what index the
		// list is at.
		// It was easier and faster to just store the index as a static and assert if called on
		// non-consecutive symbols. (Could fix it if it becomes a problem)
		Dbg_MsgAssert(p_sym==sp_last,("Non-consecutive call to GetNextSymbolTableEntry"));
	}
		
	p_sym=p_sym->mpNext;
	if (!p_sym)
	{
		// We've gone off the end of this linked list, so go onto the next entry in the hash table.
		++s_current_hash_index;
		p_sym=sp_hash_table+s_current_hash_index;
		
		if (s_current_hash_index>=(1<<NUM_HASH_BITS))
		{
			// Run out of hash table.
			s_current_hash_index=0;
			sp_last=NULL;
			return NULL;
		}	
		
		// Step through the hash table entries until a used one is found.
		while (!p_sym->mUsed)
		{
			++s_current_hash_index;
			++p_sym;
			
			if (s_current_hash_index>=(1<<NUM_HASH_BITS))
			{
				s_current_hash_index=0;
				sp_last=NULL;
				return NULL;
			}	
		}	
	}

	sp_last=p_sym;
	return p_sym;
}

float GetFloat(uint32 checksum, EAssertType assert)
{
    CSymbolTableEntry *p_entry=Resolve(checksum);
    if (!p_entry)
    {
		Dbg_MsgAssert(!assert,("Float '%s' not found.",FindChecksumName(checksum)));
        return 0.0f;
    }    
    
    switch (p_entry->mType)
    {
    case ESYMBOLTYPE_FLOAT:
        return p_entry->mFloatValue;
        break;
    case ESYMBOLTYPE_INTEGER:
        return (float)p_entry->mIntegerValue;
        break;    
    default:
        Dbg_MsgAssert(0,("Symbol '%s' of type '%s' cannot be converted to a float.",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
        return 0.0f;
        break;
    }        
	
	return 0.0f;
}

float GetFloat(const char *p_name, EAssertType assert)
{
	return GetFloat(Crc::GenerateCRCFromString(p_name),assert);
}

int GetInteger(uint32 checksum, EAssertType assert)
{
    CSymbolTableEntry *p_entry=Resolve(checksum);
    if (!p_entry)
    {
		Dbg_MsgAssert(!assert,("Integer '%s' not found.",FindChecksumName(checksum)));
        return 0;
    }    

    switch (p_entry->mType)
    {
    case ESYMBOLTYPE_FLOAT:
        return (int)p_entry->mFloatValue;
        break;
    case ESYMBOLTYPE_INTEGER:
        return p_entry->mIntegerValue;
        break;    
    default:
        Dbg_MsgAssert(0,("Symbol '%s' of type '%s' cannot be converted to an integer.",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
        return 0;
        break;
    }        

	return 0;
}

int GetInteger(const char *p_name, EAssertType assert)
{
	return GetInteger(Crc::GenerateCRCFromString(p_name),assert);
}

uint32 GetChecksum(uint32 checksum, EAssertType assert)
{
    CSymbolTableEntry *p_entry=Resolve(checksum);
    if (!p_entry)
    {
		Dbg_MsgAssert(!assert,("Checksum named '%s' not found.",FindChecksumName(checksum)));
        return 0;
    }    

    Dbg_MsgAssert(p_entry->mType==ESYMBOLTYPE_NAME,("GetChecksum: Symbol '%s' has the wrong type of '%s'",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
	return p_entry->mChecksum;	
}

uint32 GetChecksum(const char *p_name, EAssertType assert)
{
	return GetChecksum(Crc::GenerateCRCFromString(p_name),assert);
}

const char *GetString(uint32 checksum, EAssertType assert)
{
    CSymbolTableEntry *p_entry=Resolve(checksum);
    if (!p_entry)
    {
		Dbg_MsgAssert(!assert,("String named '%s' not found.",FindChecksumName(checksum)));
        return "";
    }    

    Dbg_MsgAssert(p_entry->mType==ESYMBOLTYPE_STRING || p_entry->mType==ESYMBOLTYPE_LOCALSTRING,("GetString: Symbol '%s' has the wrong type of '%s'",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
	Dbg_MsgAssert(p_entry->mpString,("NULL p_entry->mpString"));
	return p_entry->mpString;	
}

const char *GetString(const char *p_name, EAssertType assert)
{
	return GetString(Crc::GenerateCRCFromString(p_name),assert);
}

const char *GetLocalString(uint32 checksum, EAssertType assert)
{
    CSymbolTableEntry *p_entry=Resolve(checksum);
    if (!p_entry)
    {
		Dbg_MsgAssert(!assert,("Local string named '%s' not found.",FindChecksumName(checksum)));
        return "";
    }    

    Dbg_MsgAssert(p_entry->mType==ESYMBOLTYPE_LOCALSTRING || p_entry->mType==ESYMBOLTYPE_STRING,("GetLocalString: Symbol '%s' has the wrong type of '%s'",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
	Dbg_MsgAssert(p_entry->mpLocalString,("NULL p_entry->mpLocalString"));
	return p_entry->mpLocalString;	
}

const char *GetLocalString(const char *p_name, EAssertType assert)
{
	return GetLocalString(Crc::GenerateCRCFromString(p_name),assert);
}

CVector *GetVector(uint32 checksum, EAssertType assert)
{
    CSymbolTableEntry *p_entry=Resolve(checksum);
    if (!p_entry)
    {
		Dbg_MsgAssert(!assert,("Vector named '%s' not found.",FindChecksumName(checksum)));
        return NULL;
    }    

    Dbg_MsgAssert(p_entry->mType==ESYMBOLTYPE_VECTOR,("GetVector: Symbol '%s' has the wrong type of '%s'",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
	Dbg_MsgAssert(p_entry->mpVector,("NULL p_entry->mpVector"));
	return p_entry->mpVector;	
}

CVector *GetVector(const char *p_name, EAssertType assert)
{
	return GetVector(Crc::GenerateCRCFromString(p_name),assert);
}

CPair *GetPair(uint32 checksum, EAssertType assert)
{
    CSymbolTableEntry *p_entry=Resolve(checksum);
    if (!p_entry)
    {
		Dbg_MsgAssert(!assert,("Pair named '%s' not found.",FindChecksumName(checksum)));
        return NULL;
    }    

    Dbg_MsgAssert(p_entry->mType==ESYMBOLTYPE_PAIR,("GetPair: Symbol '%s' has the wrong type of '%s'",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
	Dbg_MsgAssert(p_entry->mpPair,("NULL p_entry->mpPair"));
	return p_entry->mpPair;	
}

CPair *GetPair(const char *p_name, EAssertType assert)
{
	return GetPair(Crc::GenerateCRCFromString(p_name),assert);
}

CStruct *GetStructure(uint32 checksum, EAssertType assert)
{
    CSymbolTableEntry *p_entry=Resolve(checksum);
    if (!p_entry)
    {
		Dbg_MsgAssert(!assert,("Structure named '%s' not found.",FindChecksumName(checksum)));
        return NULL;
    }    

	#ifdef	__NOPT_ASSERT__
	if (assert)
	{
		Dbg_MsgAssert(p_entry->mType==ESYMBOLTYPE_STRUCTURE,("GetStructure: Symbol '%s' has the wrong type of '%s'",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
	}
	else
	#endif
	{
		if (p_entry->mType!=ESYMBOLTYPE_STRUCTURE)
		{
			return NULL;
		}
	}		
	
	Dbg_MsgAssert(p_entry->mpStructure,("NULL p_entry->mpStructure"));
	return p_entry->mpStructure;	
}

CStruct *GetStructure(const char *p_name, EAssertType assert)
{
	return GetStructure(Crc::GenerateCRCFromString(p_name),assert);
}

CArray *GetArray(uint32 checksum, EAssertType assert)
{
    CSymbolTableEntry *p_entry=Resolve(checksum);
    if (!p_entry)
    {
		Dbg_MsgAssert(!assert,("Array named '%s' not found.",FindChecksumName(checksum)));
        return NULL;
    }    

	#ifdef	__NOPT_ASSERT__
	if (assert)
	{
		Dbg_MsgAssert(p_entry->mType==ESYMBOLTYPE_ARRAY,("GetArray: Symbol '%s' has the wrong type of '%s'",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
	}
	else
	#endif
	{
		if (p_entry->mType!=ESYMBOLTYPE_ARRAY)
		{
			return NULL;
		}
	}
			
	Dbg_MsgAssert(p_entry->mpArray,("NULL p_entry->mpArray"));
	return p_entry->mpArray;	
}

CArray *GetArray(const char *p_name, EAssertType assert)
{
	return GetArray(Crc::GenerateCRCFromString(p_name),assert);
}

bool (*GetCFunc(uint32 checksum, EAssertType assert))(CStruct *, CScript *)
{
	CSymbolTableEntry *p_entry=Resolve(checksum);
	if (!p_entry)
	{
		Dbg_MsgAssert(!assert,("GetCFunc could not find the symbol '%s'. No symbol of any type exists with that name.",FindChecksumName(checksum)));
		return NULL;
	}
		
	switch (p_entry->mType)
	{
		case ESYMBOLTYPE_CFUNCTION:
			Dbg_MsgAssert(p_entry->mpCFunction,("NULL p_entry->pCFunction ?"));
			return p_entry->mpCFunction;
			break;
			
		case ESYMBOLTYPE_MEMBERFUNCTION:
			Dbg_MsgAssert(0,("'%s' is not a C-function, it is a member function.",FindChecksumName(checksum)));
			break;
			
		default:	
			Dbg_MsgAssert(0,("'%s' is not a C-function, but has type '%s'",FindChecksumName(checksum),GetTypeName(p_entry->mType)));
			break;
	}
	
	return NULL;			
}

bool (*GetCFunc(const char *p_name, EAssertType assert))(CStruct *, CScript *)
{
	return GetCFunc(Crc::GenerateCRCFromString(p_name),assert);
}

///////////////////////////////////////////////////////////////////////////////////
// TODO: Remove these next functions at some point.
// They are only included to provide back compatibility with the old code without
// having to change thousands of occurrences.
int GetInt(uint32 checksum, bool assert)
{
	return GetInteger(checksum,assert?ASSERT:NO_ASSERT);
}

int GetInt(const char *p_name, bool assert)
{
	return GetInteger(p_name,assert?ASSERT:NO_ASSERT);
}

bool (*GetCFuncPointer(uint32 checksum, EAssertType assert))(CStruct *, CScript *)
{
	return GetCFunc(checksum, assert);
}

bool (*GetCFuncPointer(const char *p_name, EAssertType assert))(CStruct *, CScript *)
{
	return GetCFunc(p_name, assert);
}
///////////////////////////////////////////////////////////////////////////////////

} // namespace Script

