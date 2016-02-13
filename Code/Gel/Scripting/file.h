#ifndef	__SCRIPTING_FILE_H
#define	__SCRIPTING_FILE_H

#ifndef	__SCRIPTING_SCRIPTDEFS_H
#include <gel/scripting/scriptdefs.h> // For enums
#endif

namespace Script
{
	
void LoadQB(const char *p_fileName, 
			EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols=NO_ASSERT_IF_DUPLICATE_SYMBOLS);
void LoadQBFromMemory(const char* p_fileName, uint8* p_qb, EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols);
void UnloadQB(uint32 fileNameChecksum);

} // namespace Script

#endif // #ifndef	__SCRIPTING_FILE_H

