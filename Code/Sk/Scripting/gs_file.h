#ifndef	__SK_SCRIPTING_GS_FILE_H
#define	__SK_SCRIPTING_GS_FILE_H

#ifndef	__SCRIPTING_SCRIPTDEFS_H
#include <gel/scripting/scriptdefs.h> // For enums
#endif

namespace SkateScript
{
using namespace Script;

void LoadAllStartupQBFiles();
void LoadQB(const char *p_fileName, 
			EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols=NO_ASSERT_IF_DUPLICATE_SYMBOLS);
void UnloadQB(uint32 fileNameChecksum);
uint32 GenerateFileNameChecksum(const char *p_fileName);
void UnloadQB(const char *p_fileName);

} // namespace SkateScript

#endif // #ifndef	__SK_SCRIPTING_GS_FILE_H

