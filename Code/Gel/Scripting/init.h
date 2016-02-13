#ifndef	__SCRIPTING_INIT_H
#define	__SCRIPTING_INIT_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

namespace Script
{

enum
{
	MAX_CSCRIPTS=300, // If this changes, the script debugger (qdebug.exe) will need to be rebuilt.
};
	
class CStruct;
class CScript;
struct SCFunction
{
    const char *mpName;
    bool (*mpFunction)(Script::CStruct *, Script::CScript *);
};

void AllocatePools();
void DeallocatePools();
void RegisterCFunctions(SCFunction *p_cFunctions, uint32 numFunctions);
void RegisterMemberFunctions(const char **pp_memberFunctions, uint32 numFunctions);

} // namespace Script

#endif // #ifndef __SCRIPTING_INIT_H
