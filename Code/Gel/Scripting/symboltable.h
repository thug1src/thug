#ifndef	__SCRIPTING_SYMBOLTABLE_H
#define	__SCRIPTING_SYMBOLTABLE_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

#ifndef	__SCRIPTING_SYMBOLTYPE_H
#include <gel/scripting/symboltype.h> // For ESymbolType
#endif

#ifndef	__SCRIPTING_SCRIPTDEFS_H
#include <gel/scripting/scriptdefs.h> // For EAssertType
#endif

namespace Script
{

// If defined then the usage of each cfunc and member func will be counted on startup
// by scanning through all the scripts.
// Uses a bit of memory so only define on debug builds.
#ifdef __NOPT_ASSERT__
//#define COUNT_USAGE
#endif

#define NUM_HASH_BITS 12

class CPair;
class CVector;
class CArray;
class CStruct;
class CScript;

#ifdef __PLAT_WN32__
class CSymbolTableEntry
#else
class CSymbolTableEntry : public Mem::CPoolable<CSymbolTableEntry>
#endif
{
public:
	// Note: The placement of these bitfielded members is important. The CPoolable class
	// has an overhead of 1 byte, so by putting the small members here they will use up the 3
	// byte padding, keeping the size of this class down. There are 4000 or so of these, so
	// the size needs to be kept as small as possible.
	
	// If a symbol is deleted and recreated by parse.cpp then this flag will get set.
	// This is so that other code can use this to detect whether a symbol has changed due to a reload.
	// For example, file.cpp checks the script mGotReloaded flags after loading a qb, and if reloaded
	// it will then restart all existing CScript's that refer to it so that  they do not crash due to 
	// invalid pointers.
	// Similarly, if a qb gets unloaded any existing CScript's that refer to a script in it must be stopped.
	// Also, in THPS, if the symbol NodeArray gets reloaded then name prefix information must be regenerated.
	//
	uint8 mGotReloaded:1;
	
	// The symbols in the contiguous array use this flag to indicate if they are occupied.
    bool mUsed:1;
	
    uint8 mType;
	
	#ifdef COUNT_USAGE
	uint32 mUsage;
	#endif
	
	// This used to just store the upper checksum bits, because the lower bits are implied
	// by the index of this entries list in the hash table.
	// This was just to save space, because of the fairly large (8000ish) number of symbols.
	// However, it makes the LookUpSymbol function a little bit slower because of the extra bit
	// operations needed to compare, and also it makes the RemoveSymbol function more complex
	// and slower because it has to search the whole symbol table in order to find the previous
	// entry, so that the current one can be unlinked.
	// In skate4, since we have the ability to unload qb's, this should reduce the requirement
	// for such a large symbol table, so I decided to store the whole checksum instead.
	// Simpler, faster and neater! Just uses a wee bit more space. Not a huge amount though,
	// probably only 25K ish.
    uint32 mNameChecksum;
	
	// Required so that all the symbols from a particular a qb can be unloaded.
	// Also handy for debugging.
	// Could perhaps make it a 16-bit index into an array of file names rather than a checksum,
	// then it would pack next to the above mUsed and mType and they would all 3 only use up 4 bytes
	// altogether rather than 8 ...
	uint32 mSourceFileNameChecksum;

    union
    {
        int mIntegerValue;
        float mFloatValue;
        char *mpString;
        char *mpLocalString;
        CPair *mpPair;
        CVector *mpVector;
        CStruct *mpStructure;
#ifdef __PLAT_NGC__
        uint32 mScriptOffset;
#else
        uint8 *mpScript;
#endif		// __PLAT_NGC__
        bool (*mpCFunction)(CStruct *pParams, CScript *pCScript);
        CArray *mpArray;
        uint32 mChecksum; // Used when Type is ESYMBOLTYPE_NAME
		uint32 mUnion; // For when all the above need to be zeroed 
    };

    CSymbolTableEntry *mpNext;

	CSymbolTableEntry();
	#ifdef __NOPT_ASSERT__
	~CSymbolTableEntry();
	#endif

	// These cannot be defined because it would cause a cyclic dependency, because
	// a CSymbolTableEntry member function can't create things. So declare them but leave them undefined
	// so that it will not link if they are attempted to be used.
	CSymbolTableEntry( const CSymbolTableEntry& rhs );
	CSymbolTableEntry& operator=( const CSymbolTableEntry& rhs );
	
	void Reset();
};

void CreateSymbolHashTable();
void DestroySymbolHashTable();
CSymbolTableEntry *LookUpSymbol(uint32 checksum);
CSymbolTableEntry *LookUpSymbol(const char *p_name);
CSymbolTableEntry *Resolve(uint32 checksum);
CSymbolTableEntry *Resolve(const char *p_name);
void RemoveSymbol(CSymbolTableEntry *p_sym);
CSymbolTableEntry *CreateNewSymbolEntry(uint32 checksum);
CSymbolTableEntry *GetNextSymbolTableEntry(CSymbolTableEntry *p_sym=NULL);

float GetFloat(uint32 checksum, EAssertType assert=NO_ASSERT);
float GetFloat(const char *p_name, EAssertType assert=NO_ASSERT);
int GetInteger(uint32 checksum, EAssertType assert=NO_ASSERT);
int GetInteger(const char *p_name, EAssertType assert=NO_ASSERT);
uint32 GetChecksum(uint32 checksum, EAssertType assert=NO_ASSERT);
uint32 GetChecksum(const char *p_name, EAssertType assert=NO_ASSERT);
const char *GetString(uint32 checksum, EAssertType assert=NO_ASSERT);
const char *GetString(const char *p_name, EAssertType assert=NO_ASSERT);
const char *GetLocalString(uint32 checksum, EAssertType assert=NO_ASSERT);
const char *GetLocalString(const char *p_name, EAssertType assert=NO_ASSERT);
CVector *GetVector(uint32 checksum, EAssertType assert=NO_ASSERT);
CVector *GetVector(const char *p_name, EAssertType assert=NO_ASSERT);
CPair *GetPair(uint32 checksum, EAssertType assert=NO_ASSERT);
CPair *GetPair(const char *p_name, EAssertType assert=NO_ASSERT);
CStruct *GetStructure(uint32 checksum, EAssertType assert=NO_ASSERT);
CStruct *GetStructure(const char *p_name, EAssertType assert=NO_ASSERT);
CArray *GetArray(uint32 checksum, EAssertType assert=NO_ASSERT);
CArray *GetArray(const char *p_name, EAssertType assert=NO_ASSERT);
bool (*GetCFunc(uint32 checksum, EAssertType assert=NO_ASSERT))(CStruct *, CScript *);
bool (*GetCFunc(const char *p_name, EAssertType assert=NO_ASSERT))(CStruct *, CScript *);

///////////////////////////////////////////////////////////////////////////////////
// TODO: Remove these next functions at some point.
// They are only included to provide back compatibility with the old code without
// having to change thousands of occurrences of calls to GetInt
int GetInt(uint32 checksum, bool assert=true);
int GetInt(const char *p_name, bool assert=true);
bool (*GetCFuncPointer(uint32 checksum, EAssertType assert=NO_ASSERT))(CStruct *, CScript *);
bool (*GetCFuncPointer(const char *p_name, EAssertType assert=NO_ASSERT))(CStruct *, CScript *);
///////////////////////////////////////////////////////////////////////////////////

} // namespace Script

#endif // #ifndef	__SCRIPTING_SYMBOLTABLE_H

