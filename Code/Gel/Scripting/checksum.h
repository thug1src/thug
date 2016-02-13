#ifndef	__SCRIPTING_CHECKSUM_H
#define	__SCRIPTING_CHECKSUM_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

namespace Script
{

void AllocateChecksumNameLookupTables();
void DeallocateChecksumNameLookupTables();
void GetChecksumNamesBuffer(char **pp_start, char **pp_end);
void AddChecksumName(uint32 checksum, const char *pName);
const char *FindChecksumNameNULL(uint32 checksum);
const char *FindChecksumName(uint32 checksum);

} // namespace Script

#endif // #ifndef	__SCRIPTING_CHECKSUM_H

