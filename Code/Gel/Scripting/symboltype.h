#ifndef	__SCRIPTING_SYMBOLTYPE_H
#define	__SCRIPTING_SYMBOLTYPE_H

namespace Script
{
// Values for CSymbolTableEntry::mType and CComponent::mType
enum ESymbolType
{
    ESYMBOLTYPE_NONE=0,
    ESYMBOLTYPE_INTEGER,
    ESYMBOLTYPE_FLOAT,
    ESYMBOLTYPE_STRING,
    ESYMBOLTYPE_LOCALSTRING,
    ESYMBOLTYPE_PAIR,
    ESYMBOLTYPE_VECTOR,
    ESYMBOLTYPE_QSCRIPT,
    ESYMBOLTYPE_CFUNCTION,
    ESYMBOLTYPE_MEMBERFUNCTION,
    ESYMBOLTYPE_STRUCTURE,
	// ESYMBOLTYPE_STRUCTUREPOINTER is not really used any more. It is only supported as a valid
	// type that can be sent to AddComponent, which is an old CStruct member function that is
	// still supported for back compatibility. mType will never be ESYMBOLTYPE_STRUCTUREPOINTER
    ESYMBOLTYPE_STRUCTUREPOINTER,
    ESYMBOLTYPE_ARRAY,
    ESYMBOLTYPE_NAME,
	
	// These symbols are just used for memory optimization by the
	// CScriptStructure WriteToBuffer and ReadFromBuffer functions.
	ESYMBOLTYPE_INTEGER_ONE_BYTE,
	ESYMBOLTYPE_INTEGER_TWO_BYTES,
	ESYMBOLTYPE_UNSIGNED_INTEGER_ONE_BYTE,
	ESYMBOLTYPE_UNSIGNED_INTEGER_TWO_BYTES,
	ESYMBOLTYPE_ZERO_INTEGER,
	ESYMBOLTYPE_ZERO_FLOAT,
	
	// Warning! Don't exceed 256 entries, since Type is a uint8 in SSymbolTableEntry
	// New warning! Don't exceed 64 entries, because the top two bits of the symbol
	// type are used to indicate whether the name checksum has been compressed to
	// a 8 or 16 bit index, when WriteToBuffer writes out parameter names.
};

// These get masked onto the symbol type in CScriptStructure::WriteToBuffer if
// the following name checksum matches one in the lookup table. (Defined in compress.q)
#define MASK_8_BIT_NAME_LOOKUP (1<<7)
#define MASK_16_BIT_NAME_LOOKUP (1<<6)

const char *GetTypeName(uint8 type);

// TODO: Remove these at some point, they are just to allow compilation of the existing game code
// without having to change all the occurrences.
#define ESYMBOLTYPE_NONE                       Script::ESYMBOLTYPE_NONE                      
#define ESYMBOLTYPE_INTEGER                    Script::ESYMBOLTYPE_INTEGER                   
#define ESYMBOLTYPE_FLOAT                      Script::ESYMBOLTYPE_FLOAT                     
#define ESYMBOLTYPE_STRING                     Script::ESYMBOLTYPE_STRING                    
#define ESYMBOLTYPE_LOCALSTRING                Script::ESYMBOLTYPE_LOCALSTRING               
#define ESYMBOLTYPE_PAIR                       Script::ESYMBOLTYPE_PAIR                      
#define ESYMBOLTYPE_VECTOR                     Script::ESYMBOLTYPE_VECTOR                    
#define ESYMBOLTYPE_QSCRIPT                    Script::ESYMBOLTYPE_QSCRIPT                   
#define ESYMBOLTYPE_CFUNCTION                  Script::ESYMBOLTYPE_CFUNCTION                 
#define ESYMBOLTYPE_MEMBERFUNCTION             Script::ESYMBOLTYPE_MEMBERFUNCTION            
#define ESYMBOLTYPE_STRUCTURE                  Script::ESYMBOLTYPE_STRUCTURE                 
#define ESYMBOLTYPE_STRUCTUREPOINTER           Script::ESYMBOLTYPE_STRUCTUREPOINTER          
#define ESYMBOLTYPE_ARRAY                      Script::ESYMBOLTYPE_ARRAY                     
#define ESYMBOLTYPE_NAME                       Script::ESYMBOLTYPE_NAME                      
#define ESYMBOLTYPE_INTEGER_ONE_BYTE           Script::ESYMBOLTYPE_INTEGER_ONE_BYTE          
#define ESYMBOLTYPE_INTEGER_TWO_BYTES          Script::ESYMBOLTYPE_INTEGER_TWO_BYTES         
#define ESYMBOLTYPE_UNSIGNED_INTEGER_ONE_BYTE  Script::ESYMBOLTYPE_UNSIGNED_INTEGER_ONE_BYTE 
#define ESYMBOLTYPE_UNSIGNED_INTEGER_TWO_BYTES Script::ESYMBOLTYPE_UNSIGNED_INTEGER_TWO_BYTES
#define ESYMBOLTYPE_ZERO_INTEGER               Script::ESYMBOLTYPE_ZERO_INTEGER              
#define ESYMBOLTYPE_ZERO_FLOAT                 Script::ESYMBOLTYPE_ZERO_FLOAT                



}

#endif // #ifndef	__SCRIPTING_SYMBOLTYPE_H
