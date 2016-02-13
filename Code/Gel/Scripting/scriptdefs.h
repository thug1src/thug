#ifndef	__SCRIPTING_SCRIPTDEFS_H
#define	__SCRIPTING_SCRIPTDEFS_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

namespace Script
{
// A checksum of 0, used to represent no-name.    
#define NO_NAME ((uint32)0)

enum EAssertType
{
	NO_ASSERT=0,
	ASSERT
};

enum EBoolAssertIfDuplicateSymbols
{
	NO_ASSERT_IF_DUPLICATE_SYMBOLS=0,
	ASSERT_IF_DUPLICATE_SYMBOLS
};
	
// So that the old code still compiles without having to change a million things.
#define CScriptStructure CStruct
#define SPair CPair
#define NONAME NO_NAME

} // namespace Script

#endif // #ifndef	__SCRIPTING_SCRIPTDEFS_H
