///////////////////////////////////////////////////////////////////////////////////////
//
// symboltype.cpp		KSH 22 Oct 2001
//
// Just a utility function for printing a type name.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <core/defines.h>
#include <gel/scripting/symboltype.h>

namespace Script
{

#ifdef __NOPT_ASSERT__
const char *GetTypeName(uint8 type)
{
	switch (type)
	{
	case ESYMBOLTYPE_NONE:
		return "None";
		break;
	case ESYMBOLTYPE_INTEGER:
		return "Integer";
		break;
    case ESYMBOLTYPE_FLOAT:
		return "Float";
		break;
    case ESYMBOLTYPE_STRING:
		return "String";
		break;
    case ESYMBOLTYPE_LOCALSTRING:
		return "Local String";
		break;
    case ESYMBOLTYPE_PAIR:
		return "Pair";
		break;
    case ESYMBOLTYPE_VECTOR:
		return "Vector";
		break;
    case ESYMBOLTYPE_QSCRIPT:
		return "Script";
		break;
    case ESYMBOLTYPE_CFUNCTION:
		return "C-Function";
		break;
    case ESYMBOLTYPE_MEMBERFUNCTION:
		return "Member Function";
		break;
    case ESYMBOLTYPE_STRUCTURE:
		return "Structure";
		break;
    case ESYMBOLTYPE_STRUCTUREPOINTER:
		return "Structure Pointer";
		break;
    case ESYMBOLTYPE_ARRAY:
		return "Array";
		break;
    case ESYMBOLTYPE_NAME:
		return "Name";
		break;
	default:
		return "Unknown";
		break;		
	}
}
#else
const char *GetTypeName(uint8 type)
{
	return "(Hey, don't use GetTypeName on the CD!)";
}
#endif

} // namespace Script

