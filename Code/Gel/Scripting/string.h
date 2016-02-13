#ifndef	__SCRIPTING_STRING_H
#define	__SCRIPTING_STRING_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

namespace Script
{

void AllocatePermanentStringHeap(uint32 maxSize, uint32 maxStrings);
void DeallocatePermanentStringHeap();

void UsePermanentStringHeap();
void UseRegularStringHeap();
	
char *CreateString(const char *p_string);
void DeleteString(char *p_string);

void SetScriptString(uint32 n, const char *p_string);
char* GetScriptString(uint32 n);

} // namespace Script

#endif // #ifndef	__SCRIPTING_STRING_H
