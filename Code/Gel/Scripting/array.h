#ifndef	__SCRIPTING_ARRAY_H
#define	__SCRIPTING_ARRAY_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

#ifndef	__SCRIPTING_SYMBOLTYPE_H
#include <gel/scripting/symboltype.h> // For ESymbolType
#endif

namespace Script
{

class CVector;
class CPair;
class CStruct;

#ifdef __PLAT_WN32__
class CArray
#else
class CArray : public Mem::CPoolable<CArray>
#endif
{
	// Pointer to the array data.
	union
	{
		// Generic pointer.
		// Used when calling Mem::Free.
		uint32 *mp_array_data;
		
		int *mp_integers;
		float *mp_floats;
		uint32 *mp_checksums;
		
		char **mpp_strings;
		char **mpp_local_strings;
		CPair **mpp_pairs;
		CVector **mpp_vectors;
		CStruct **mpp_structures;
		CArray **mpp_arrays;
		
		// In the case of the array containing only 1 element, the element itself is
		// stored here, rather than allocating a block of memory for it.
		// This is a memory optimization.
		// Each memory block uses 16 bytes for the header, and the data is padded to
		// occupy 16 bytes. So in the case of an array of 1 element this saves 32 bytes.
		// There are lots of arrays of 1 element, eg the links arrays in each node of 
		// the NodeArray often only contain 1 link.
		int m_integer;
		float m_float;
		uint32 m_checksum;
		char *mp_string;
		char *mp_local_string;
		CPair *mp_pair;
		CVector *mp_vector;
		CStruct *mp_structure;
		CArray *mp_array;
		// Used to zero the single element.
		uint32 m_union;
	};

	// The type of the things in the array.
	ESymbolType m_type;
	
	// The number of items in the array.
	uint32 m_size;

public:
	CArray();
	~CArray();

	// These cannot be defined because it would cause a cyclic dependency, because
	// a CArray member function can't create things. So declare them but leave them undefined
	// so that it will not link if they are attempted to be used.
	CArray( const CArray& rhs );
	CArray& operator=( const CArray& rhs );
	
	// This is used when interpreting switch statements.
	bool operator==( const CArray& v ) const;
	bool operator!=( const CArray& v ) const;
	
	void Clear();
	void SetSizeAndType(int size, ESymbolType type);
	void Resize(uint32 newSize);
	
	// TODO: Remove later. Only included for back compatibility.
	void SetArrayType(int size, ESymbolType type) {SetSizeAndType(size,type);}
	
	void 	  SetString(uint32 index, char *p_string);
	void SetLocalString(uint32 index, char *p_string);
	void 	 SetInteger(uint32 index, int int_val);
	void 	   SetFloat(uint32 index, float float_val);
	void 	SetChecksum(uint32 index, uint32 checksum);
	void 	  SetVector(uint32 index, CVector *p_vector);
	void 		SetPair(uint32 index, CPair *p_pair);
	void   SetStructure(uint32 index, CStruct *p_struct);
	void 	   SetArray(uint32 index, CArray *p_array);

	char 			*GetString(uint32 index) const;
	char 	   *GetLocalString(uint32 index) const;
	int 			GetInteger(uint32 index) const;
	float 			  GetFloat(uint32 index) const;
	uint32 		   GetChecksum(uint32 index) const;
	CVector			*GetVector(uint32 index) const;
	CPair 			  *GetPair(uint32 index) const;
	CStruct 	 *GetStructure(uint32 index) const;
	CArray			 *GetArray(uint32 index) const;

	////////////////////////////////////////////////////////////////////////////////////
	// TODO: Remove these later, only needed for back compatibility.
	uint32	   GetNameChecksum(uint32 index) const {return GetChecksum(index);}
	int 				GetInt(uint32 index) const {return GetInteger(index);}
	////////////////////////////////////////////////////////////////////////////////////
	
	uint32 		GetSize() const {return m_size;};
	ESymbolType GetType() const {return m_type;};
	
	// Needed by CleanUpArray and CopyArray in struct.cpp so that they can
	// quickly scan through the array data without having to use the access functions
	// to get each element.
	uint32 *GetArrayPointer() const;
};

} // namespace Script

#endif // #ifndef	__SCRIPTING_ARRAY_H
