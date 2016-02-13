///////////////////////////////////////////////////////////////////////////////////////
//
// array.cpp		KSH 22 Oct 2001
//
// CArray class member functions.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/array.h>

DefinePoolableClass(Script::CArray);

namespace Script
{

CArray::CArray()
{
	// Initialise everything. CArray is not derived from CClass so we don't get
	// the auro-zeroing.
	m_type=ESYMBOLTYPE_NONE;
	mp_array_data=NULL;
	m_size=0;
}

CArray::~CArray()
{
	Clear();
}

bool CArray::operator==( const CArray& v ) const
{
	// TODO ...
	#ifdef __NOPT_ASSERT__
	printf("CArray comparisons are not supported yet ... implement when needed\n");
	#endif
	return false;
}

bool CArray::operator!=( const CArray& v ) const
{
	return !(*this==v);
}

// Deletes the array buffer if it exists, asserting if it contains any non-NULL pointers.
// Sets type to NONE and size to 0.
void CArray::Clear()
{
	if (m_size==1)
	{
		// Memory optimization:
		// Special case for size 1. In this case, no memory block has been allocated.
		
		if (m_union)
		{
			// The element is not zero ...
			
			#ifdef __NOPT_ASSERT__
			// Check that no references to things remain in the array.
			switch (m_type)
			{
				case ESYMBOLTYPE_INTEGER:
				case ESYMBOLTYPE_FLOAT:
				case ESYMBOLTYPE_NAME:
					// No need for the user to have zeroed these.
					break;
					
				case ESYMBOLTYPE_STRING:
				case ESYMBOLTYPE_LOCALSTRING:
				case ESYMBOLTYPE_PAIR:
				case ESYMBOLTYPE_VECTOR:
				case ESYMBOLTYPE_STRUCTURE:
				case ESYMBOLTYPE_ARRAY:
				{
					// The array contains a pointer to something.
					// The CArray cannot delete it itself because this would cause cyclic dependencies.
					Dbg_MsgAssert(0,("Tried to delete a CArray that still contains non-NULL data: size=%d type='%s'",m_size,GetTypeName(m_type)));
					break;
				}	
				
				default:
					Dbg_MsgAssert(0,("Bad CArray::m_type of '%s'",GetTypeName(m_type)));
					break;
			}		
			#endif
		
			m_union=0;
		}	
	}
	else
	{
		if (mp_array_data)
		{
			#ifdef __NOPT_ASSERT__
			// Check that no references to things remain in the array.
			switch (m_type)
			{
				case ESYMBOLTYPE_INTEGER:
				case ESYMBOLTYPE_FLOAT:
				case ESYMBOLTYPE_NAME:
					// No need for the user to have zeroed these.
					break;
					
				case ESYMBOLTYPE_STRING:
				case ESYMBOLTYPE_LOCALSTRING:
				case ESYMBOLTYPE_PAIR:
				case ESYMBOLTYPE_VECTOR:
				case ESYMBOLTYPE_STRUCTURE:
				case ESYMBOLTYPE_ARRAY:
				{
					// The array is of pointers, so make sure that the user of CArray has deleted and zeroed these before deleting the array.
					// The CArray cannot delete them itself because this would cause cyclic dependencies.
					for (uint32 i=0; i<m_size; ++i)
					{
						Dbg_MsgAssert(mp_array_data[i]==0,("Tried to delete a CArray that still contains non-NULL data: size=%d type='%s'",m_size,GetTypeName(m_type)));
					}
					break;
				}	
				
				default:
					Dbg_MsgAssert(0,("Bad CArray::m_type of '%s'",GetTypeName(m_type)));
					break;
			}		
			#endif
			
			Mem::Free(mp_array_data);
			mp_array_data=NULL;
		}	
	}
		
	m_type=ESYMBOLTYPE_NONE;
	m_size=0;
}

// Calls Clear, sets the size and type, allocates the buffer if necessary and initialises it to all zeroes.
void CArray::SetSizeAndType(int size, ESymbolType type)
{
	switch (type)
	{
		case ESYMBOLTYPE_INTEGER:
		case ESYMBOLTYPE_FLOAT:
		case ESYMBOLTYPE_NAME:
		case ESYMBOLTYPE_STRING:
		case ESYMBOLTYPE_LOCALSTRING:
		case ESYMBOLTYPE_PAIR:
		case ESYMBOLTYPE_VECTOR:
		case ESYMBOLTYPE_STRUCTURE:
		case ESYMBOLTYPE_ARRAY:
			break;
		
		case ESYMBOLTYPE_NONE:
			Dbg_MsgAssert(size==0,("Array type of NONE with non-zero size sent to CArray::SetSizeAndType"));
			break;
			
		default:
			Dbg_MsgAssert(0,("Bad type of '%s' sent to CArray::SetSizeAndType",GetTypeName(type)));
			break;
	}		
	
	Clear();
	m_type=type;
	m_size=size;

	if (size)
	{
		if (size==1)
		{
			Dbg_MsgAssert(m_union==0,("m_union not zero ?"));
			// Nothing to do. No memory block is allocated for arrays of 1 element, to save memory.
		}
		else
		{
			Dbg_MsgAssert(mp_array_data==NULL,("mp_array_data not NULL ?"));
			
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
			mp_array_data=(uint32*)Mem::Malloc(m_size*sizeof(uint32));
			Mem::Manager::sHandle().PopContext();
			
			uint32 *p_long_word=mp_array_data;
			for (uint32 i=0; i<m_size; ++i)
			{
				*p_long_word++=0;
			}	
		}	
	}
	else
	{
		// Make all zero size arrays have type none.
		// Seems a reasonable thing to do, and in particular this fixed an assert in WriteToBuffer
		// after CSkaterCareer::WriteIntoStructure had added an empty array of structures. (The Gaps array)
		m_type=ESYMBOLTYPE_NONE;
	}	
}

void CArray::Resize(uint32 newSize)
{
	if (newSize==m_size)
	{
		// Nothing to do
		return;
	}	
	Dbg_MsgAssert(newSize>m_size,("Tried to resize CArray to a smaller size, not supported yet ..."));
	// TODO: Make it able to make the CArray smaller, if a need arises. To do, factor out some of the
	// code from CleanUpArray so that the leftover bit can be cleaned up.
	Dbg_MsgAssert(newSize>1,("Resizing arrays to size 1 not supported yet ..."));
	// TODO: Support the above if need be. Need to not allocate a new buffer in that case.
	
	// Allocate the new buffer.
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	uint32 *p_new_buffer=(uint32*)Mem::Malloc(newSize*sizeof(uint32));
	Mem::Manager::sHandle().PopContext();
	
	// Copy the contents of the old buffer into the new.
	uint32 *p_source=GetArrayPointer();
	// Note: Does not support resizing zero size arrays because it does not know what the type of the 
	// new array should be.
	Dbg_MsgAssert(p_source,("NULL array pointer ?"));
	uint32 *p_dest=p_new_buffer;
	uint32 i;
	for (i=0; i<m_size; ++i)
	{
		*p_dest++=*p_source++;
	}
	// Zero the remainder of the new buffer.
	uint32 remainder=newSize-m_size;
	for (i=0; i<remainder; ++i)
	{
		*p_dest++=0;
	}
	
	// Only free mp_array_data if the old size was bigger than 1. 
	// mp_array_data is not allocated for sizes of 1 as a memory optimization.
	if (m_size > 1)
	{
		Mem::Free(mp_array_data);
	}	
	mp_array_data=p_new_buffer;
	
	m_size=newSize;
	Dbg_MsgAssert(m_size>1,("Expected array size to be > 1 ??")); // Just to be sure
}

uint32 *CArray::GetArrayPointer() const
{
	if (m_size==1)
	{
		return (uint32*)&m_union;
	}
	return mp_array_data;
}

void CArray::SetString(uint32 index, char *p_string)
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_STRING,("Called CArray::SetString when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::SetString, m_size=%d",index,m_size));
	if (m_size==1)
	{
		Dbg_MsgAssert(mp_string==NULL,("mp_string expected to be NULL"));
		mp_string=p_string;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		Dbg_MsgAssert(mpp_strings[index]==NULL,("Non-NULL pointer in mpp_strings[index]"));
		mpp_strings[index]=p_string;
	}	
}

void CArray::SetLocalString(uint32 index, char *p_string)
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_LOCALSTRING,("Called CArray::SetLocalString when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::SetLocalString, m_size=%d",index,m_size));
	if (m_size==1)
	{
		Dbg_MsgAssert(mp_local_string==NULL,("mp_local_string expected to be NULL"));
		mp_local_string=p_string;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		Dbg_MsgAssert(mpp_local_strings[index]==NULL,("Non-NULL pointer in mpp_local_strings[index]"));
		mpp_local_strings[index]=p_string;
	}	
}

void CArray::SetInteger(uint32 index, int int_val)
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_INTEGER,("Called CArray::SetInteger when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::SetInteger, m_size=%d",index,m_size));
	if (m_size==1)
	{
		m_integer=int_val;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		mp_integers[index]=int_val;
	}	
}

void CArray::SetFloat(uint32 index, float float_val)
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_FLOAT,("Called CArray::SetFloat when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::SetFloat, m_size=%d",index,m_size));
	if (m_size==1)
	{
		m_float=float_val;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		mp_floats[index]=float_val;
	}	
}

void CArray::SetChecksum(uint32 index, uint32 checksum)
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_NAME,("Called CArray::SetChecksum when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::SetChecksum, m_size=%d",index,m_size));
	if (m_size==1)
	{
		m_checksum=checksum;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		mp_checksums[index]=checksum;
	}	
}

void CArray::SetVector(uint32 index, CVector *p_vector)
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_VECTOR,("Called CArray::SetVector when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::SetVector, m_size=%d",index,m_size));
	if (m_size==1)
	{
		Dbg_MsgAssert(mp_vector==NULL,("mp_vector expected to be NULL"));
		mp_vector=p_vector;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		Dbg_MsgAssert(mpp_vectors[index]==NULL,("Non-NULL pointer in mpp_vectors[index]"));
		mpp_vectors[index]=p_vector;
	}	
}

void CArray::SetPair(uint32 index, CPair *p_pair)
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_PAIR,("Called CArray::SetPair when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::SetPair, m_size=%d",index,m_size));
	if (m_size==1)
	{
		Dbg_MsgAssert(mp_pair==NULL,("mp_pair expected to be NULL"));
		mp_pair=p_pair;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		Dbg_MsgAssert(mpp_pairs[index]==NULL,("Non-NULL pointer in mpp_pairs[index]"));
		mpp_pairs[index]=p_pair;
	}	
}

void CArray::SetStructure(uint32 index, CStruct *p_struct)
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_STRUCTURE,("Called CArray::SetStructure when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::SetStructure, m_size=%d",index,m_size));
	if (m_size==1)
	{
		Dbg_MsgAssert(mp_structure==NULL,("mp_structure expected to be NULL"));
		mp_structure=p_struct;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		Dbg_MsgAssert(mpp_structures[index]==NULL,("Non-NULL pointer in mpp_structures[index]"));
		mpp_structures[index]=p_struct;
	}	
}

void CArray::SetArray(uint32 index, CArray *p_array)
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_ARRAY,("Called CArray::SetArray when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::SetArray, m_size=%d",index,m_size));
	if (m_size==1)
	{
		Dbg_MsgAssert(mp_array==NULL,("mp_array expected to be NULL"));
		mp_array=p_array;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		Dbg_MsgAssert(mpp_arrays[index]==NULL,("Non-NULL pointer in mpp_arrays[index]"));
		mpp_arrays[index]=p_array;
	}	
}

char *CArray::GetString(uint32 index) const
{
	// The game code views local strings & strings as the same type.
	if (m_type==ESYMBOLTYPE_LOCALSTRING)
	{
		return GetLocalString(index);
	}	
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_STRING,("Called CArray::GetString when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::GetString, m_size=%d",index,m_size));
	if (m_size==1)
	{
		return mp_string;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		return mpp_strings[index];
	}	
}

char *CArray::GetLocalString(uint32 index) const
{
	if (m_type==ESYMBOLTYPE_STRING)
	{
		return GetString(index);
	}	
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_LOCALSTRING,("Called CArray::GetLocalString when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::GetLocalString, m_size=%d",index,m_size));
	if (m_size==1)
	{
		return mp_local_string;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		return mpp_local_strings[index];
	}	
}

int CArray::GetInteger(uint32 index) const
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_INTEGER,("Called CArray::GetInteger when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::GetInteger, m_size=%d",index,m_size));
	if (m_size==1)
	{
		return m_integer;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		return mp_integers[index];
	}	
}

float CArray::GetFloat(uint32 index) const
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_FLOAT || m_type==ESYMBOLTYPE_INTEGER,("Called CArray::GetFloat when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::GetFloat, m_size=%d",index,m_size));
	
	if (m_size==1)
	{
		if (m_type==ESYMBOLTYPE_FLOAT)
		{
			return m_float;
		}
		else
		{
			return m_integer;
		}	
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		
		if (m_type==ESYMBOLTYPE_FLOAT)
		{
			return mp_floats[index];
		}	
		else
		{
			return mp_integers[index];	
		}	
	}	
}

uint32 CArray::GetChecksum(uint32 index) const
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_NAME,("Called CArray::GetChecksum when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::GetChecksum, m_size=%d",index,m_size));
	if (m_size==1)
	{
		return m_checksum;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		return mp_checksums[index];
	}	
}

CVector	*CArray::GetVector(uint32 index) const
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_VECTOR,("Called CArray::GetVector when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::GetVector, m_size=%d",index,m_size));
	if (m_size==1)
	{
		return mp_vector;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		return mpp_vectors[index];
	}	
}

CPair *CArray::GetPair(uint32 index) const
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_PAIR,("Called CArray::GetPair when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::GetPair, m_size=%d",index,m_size));
	if (m_size==1)
	{
		return mp_pair;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		return mpp_pairs[index];
	}	
}

CStruct *CArray::GetStructure(uint32 index) const
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_STRUCTURE,("Called CArray::GetStructure when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::GetStructure, m_size=%d",index,m_size));
	if (m_size==1)
	{
		return mp_structure;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		return mpp_structures[index];
	}	
}

CArray *CArray::GetArray(uint32 index) const
{
	Dbg_MsgAssert(m_type==ESYMBOLTYPE_ARRAY,("Called CArray::GetArray when m_type was '%s'",GetTypeName(m_type)));
	Dbg_MsgAssert(index<m_size,("Bad index of %d sent to CArray::GetArray, m_size=%d",index,m_size));
	if (m_size==1)
	{
		return mp_array;
	}
	else
	{
		Dbg_MsgAssert(mp_array_data,("NULL mp_array_data ?"));
		return mpp_arrays[index];
	}	
}

} // namespace Script

