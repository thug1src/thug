#ifndef	__SCRIPTING_PARSE_H
#define	__SCRIPTING_PARSE_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef	__SCRIPTING_SCRIPTDEFS_H
#include <gel/scripting/scriptdefs.h> // For enums
#endif

#ifndef __SCRIPTING_TOKENS_H
#include <gel/scripting/tokens.h>
#endif

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

#ifdef __PLAT_NGC__
#define SCRIPT_HEADER_SIZE 32
#else
#define SCRIPT_HEADER_SIZE 12
#endif

namespace Script
{

class CStruct;
class CComponent;
class CSymbolTableEntry;
class CScript;

// Used when extracting 4 bytes from a .qb, where the 4 bytes could represent
// different types of thing. (Saves messy casting)
struct S4Bytes
{
    union
    {
        uint32 mChecksum;
		uint32 mUInt;
        int mInt;
        float mFloat;
        bool (*mpCFunction)(CStruct *pParams, CScript *pCScript);
    };
};

// Same, but for 2 bytes.
struct S2Bytes
{
    union
    {
		uint16 mUInt;
        sint16 mInt;
    };
};

// Gets 4 bytes from p_long, which may not be long word aligned.
S4Bytes Read4Bytes(const uint8 *p_long);
S2Bytes Read2Bytes(const uint8 *p_short);
uint8 *Write4Bytes(uint8 *p_buffer, uint32 val);
uint8 *Write4Bytes(uint8 *p_buffer, float floatVal);
uint8 *Write2Bytes(uint8 *p_buffer, uint16 val);

#define MAX_STORED_RANDOMS 100

#define MAKE_NEW_FIRST_DIFFER_FROM_OLD_LAST true
#ifdef __PLAT_WN32__
class CStoredRandom
#else
class CStoredRandom : public Mem::CPoolable<CStoredRandom>
#endif
{
public:	
	CStoredRandom();
	~CStoredRandom();
	
	// These identify which Random operator this 'belongs' to.
	// mpToken alone is not enough because there may be two instances of a CScript running
	// that script.
	CScript *mpScript;
	const uint8 *mpToken;
	// These are also stored to further verify that this is the correct one if a match for
	// mpToken is found. They could differ due to a script reload.
	uint16 mNumItems;
	EScriptToken mType;
	
	// The following members are used by the RandomPermute operator
	uint16 mCurrentIndex;
	
	#define MAX_RANDOM_INDICES 64
	uint8 mpIndices[MAX_RANDOM_INDICES];

	void InitIndices();
	void RandomizeIndices(bool makeNewFirstDifferFromOldLast=false);
	
	// This is used by the RandomNoRepeat operator
	uint32 mLastIndex;

	
	CStoredRandom *mpNext;
	CStoredRandom *mpPrevious;
};

void ReleaseStoredRandoms(CScript *p_script);

// Used by CScript::Update
uint8 *SkipToken(uint8 *p_token);
uint8 *SkipEndOfLines(uint8 *p_token);
uint8 *DoAnyRandomsOrJumps(uint8 *p_token);

uint8 *SkipOverScript(uint8 *p_token);

void EnableExpressionEvaluatorErrorChecking();
void DisableExpressionEvaluatorErrorChecking();
uint8 *Evaluate(uint8 *p_token, CStruct *p_args, CComponent *p_result);
uint8 *FillInComponentUsingQB(uint8 *p_token, CStruct *p_args, CComponent *p_comp);

// Exported because needed by CScript when generating the structure of function 
// parameters when executing each line of script.
uint8 *AddComponentsUntilEndOfLine(CStruct *p_dest, uint8 *p_token, CStruct *p_args=NULL);

// Used by UnloadQB in file.cpp
void CleanUpAndRemoveSymbol(CSymbolTableEntry *p_sym);
// TODO: Remove? Currently used in cfuncs.cpp to delete the NodeArray, but now that we
// have the ability to unload qb files that should not be necessary any more.
// Note: Just found it is also needed in parkgen.cpp, so don't remove after all. 
void CleanUpAndRemoveSymbol(uint32 checksum);
void CleanUpAndRemoveSymbol(const char *p_symbolName);


// Used by various asserts in script.cpp
int GetLineNumber(uint8 *p_token);
const char *GetSourceFile(uint8 *p_token);
bool PointsIntoScript(uint8 *p_token, uint8 *p_script);

void CheckForPossibleInfiniteLoops(uint32 scriptName, uint8 *p_token, const char *p_fileName);

const char *GetChecksumNameFromLastQB(uint32 checksum);
void RemoveChecksumNameLookupHashTable();
uint8 *SkipToStartOfNextLine(uint8 *p_token);
void PreProcessScript(uint8 *p_token);


// Called from LoadQB in file.cpp

// By default the allocateChecksumNameLookupTable flag is set to true because the table is needed when
// generating prefix info for the nodearray, and we don't know whether the qb contains the nodearray until
// after finishing parsing it.
// The flag can be set to false when we know that checksum lookup info won't be needed & we're short of memory.
// (Eg, Gary uses this when loading qb's for the cutscenes)
void ParseQB(const char *p_fileName, uint8 *p_qb, EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols=NO_ASSERT_IF_DUPLICATE_SYMBOLS, bool allocateChecksumNameLookupTable=true);

bool ScriptContainsName(uint8 *p_script, uint32 searchName);
bool ScriptContainsAnyOfTheNames(uint32 scriptName, uint32 *p_names, int numNames);

} // namespace Script

#endif // #ifndef	__SCRIPTING_PARSE_H
