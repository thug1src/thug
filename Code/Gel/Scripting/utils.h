#ifndef	__SCRIPTING_UTILS_H
#define	__SCRIPTING_UTILS_H

#ifndef	__SCRIPTING_SCRIPTDEFS_H
#include <gel/scripting/scriptdefs.h> // For EAssertType
#endif

namespace Script
{

class CArray;
class CStruct;
class CComponent;

void PrintContents(const CArray *p_array, int indent=0);
void PrintContents(const CStruct *p_structure, int indent=0);

uint32 WriteToBuffer(CStruct *p_structure, uint8 *p_buffer, uint32 bufferSize, EAssertType assert=ASSERT);
uint32 CalculateBufferSize(CStruct *p_structure, uint32 tempBufferSize=100000);
uint8 *ReadFromBuffer(CStruct *p_structure, uint8 *p_buffer);

uint32 WriteToBuffer(CArray *p_array, uint8 *p_buffer, uint32 bufferSize, EAssertType assert=ASSERT);
uint32 CalculateBufferSize(CArray *p_array);
uint8 *ReadFromBuffer(CArray *p_array, uint8 *p_buffer);

// This is used in eval.cpp, when evaluating foo[3] say.
// Copies the array element indicated by index into the passed component.
// The type of p_comp may be ESYMBOLTYPE_NONE if the type is not supported yet by not being in
// the switch statement.
void CopyArrayElementIntoComponent(CArray *p_array, uint32 index, CComponent *p_comp);
void ResolveNameComponent(CComponent *p_comp);

}

#endif // #ifndef	__SCRIPTING_UTILS_H

