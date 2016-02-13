///////////////////////////////////////////////////////////////////////////////////////
//
// struct.cpp		KSH 22 Oct 2001
//
// CStruct class member functions.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/struct.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/component.h>
#include <gel/scripting/string.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <core/crc.h>
#include <core/math.h>
#include <core/math/vector.h>

							 
//#define	__DEBUG_STRUCT_ALLOCS
							 
// This does technically cause a cyclic dependency, but what the heck, it's only needed
// for some debugging. (Making the Get function print the contents of the struct in
// the event of not finding the required parameter)
#include <gel/scripting/utils.h>

DefinePoolableClass(Script::CStruct);

namespace Script
{

#ifdef	__DEBUG_STRUCT_ALLOCS
#define	MAX_LAST	200
static int head = 0;
static bool init_last = true;
static CStruct * last[MAX_LAST];
#endif

void DumpLastStructs()
{
#ifdef	__DEBUG_STRUCT_ALLOCS
	for (int i=0;i<MAX_LAST;i++)
	{
		if (last[i])
		{
			printf ("Final Structure %d\n",i);
			PrintContents(last[i]);
		}
	}
#endif
}


// Deletes any entity pointed to by p_comp, so that p_comp can safely be deleted afterwards.
void CleanUpComponent(CComponent *p_comp)
{
	Dbg_MsgAssert(p_comp,("NULL p_comp"));
	
	switch (p_comp->mType)
	{
		case ESYMBOLTYPE_STRING:
		case ESYMBOLTYPE_LOCALSTRING:
			if (p_comp->mpString)
			{
				DeleteString(p_comp->mpString);
			}
			break;
		case ESYMBOLTYPE_PAIR:
			if (p_comp->mpPair)
			{
				delete p_comp->mpPair;
			}
			break;
		case ESYMBOLTYPE_VECTOR:
			if (p_comp->mpVector)
			{
				delete p_comp->mpVector;
			}
			break;
		case ESYMBOLTYPE_STRUCTURE:
			if (p_comp->mpStructure)
			{
				delete p_comp->mpStructure;
			}
			break;
		case ESYMBOLTYPE_ARRAY:
			if (p_comp->mpArray)
			{
				CleanUpArray(p_comp->mpArray);
				delete p_comp->mpArray;
			}
			break;
		case ESYMBOLTYPE_QSCRIPT:
            if (p_comp->mpScript)
			{
				Mem::Free(p_comp->mpScript);
			}
			p_comp->mScriptSize=0;
			break;	
		default:
			break;
	}	
	// This will zero the union, which includes all the above pointers.
	p_comp->mUnion=0;
	
	p_comp->mType=ESYMBOLTYPE_NONE;
}

// Copies the contents of p_source into p_dest, but without copying any pointers over.
// Eg, if p_source contains an array, a new array will be created for p_dest.
void CopyComponent(CComponent *p_dest, const CComponent *p_source)
{
	Dbg_MsgAssert(p_dest,("NULL p_dest"));
	Dbg_MsgAssert(p_source,("NULL p_source"));

	// Make sure p_dest is cleaned up first.	
	if (p_dest->mUnion) // This 'if' is just a small speed optimization to avoid an unnecessay call
	{
		CleanUpComponent(p_dest);
	}	
	
	p_dest->mType=p_source->mType;
	p_dest->mNameChecksum=p_source->mNameChecksum;
	
	switch (p_source->mType)
	{
	case ESYMBOLTYPE_NONE:
		break;
		
    case ESYMBOLTYPE_INTEGER:
    case ESYMBOLTYPE_FLOAT:
	case ESYMBOLTYPE_NAME:
		p_dest->mUnion=p_source->mUnion;
		break;
		
    case ESYMBOLTYPE_STRING:
		Dbg_MsgAssert(p_source->mpString,("NULL p_source->mpString ?"));
		p_dest->mpString=CreateString(p_source->mpString);
		break;
		
    case ESYMBOLTYPE_LOCALSTRING:
		Dbg_MsgAssert(p_source->mpLocalString,("NULL p_source->mpLocalString ?"));
		p_dest->mpLocalString=CreateString(p_source->mpLocalString);
		break;
		
    case ESYMBOLTYPE_PAIR:
		Dbg_MsgAssert(p_source->mpPair,("NULL p_source->mpPair ?"));
		p_dest->mpPair=new CPair;
		p_dest->mpPair->mX=p_source->mpPair->mX;
		p_dest->mpPair->mY=p_source->mpPair->mY;
		break;
		
    case ESYMBOLTYPE_VECTOR:
		Dbg_MsgAssert(p_source->mpVector,("NULL p_source->mpVector ?"));
		p_dest->mpVector=new CVector;
		p_dest->mpVector->mX=p_source->mpVector->mX;
		p_dest->mpVector->mY=p_source->mpVector->mY;
		p_dest->mpVector->mZ=p_source->mpVector->mZ;
		break;
		
    case ESYMBOLTYPE_STRUCTURE:
		Dbg_MsgAssert(p_source->mpStructure,("NULL p_source->mpStructure ?"));
		p_dest->mpStructure=new CStruct;
		*p_dest->mpStructure=*p_source->mpStructure;
		break;
		
    case ESYMBOLTYPE_ARRAY:
		Dbg_MsgAssert(p_source->mpArray,("NULL p_source->mpArray ?"));
		p_dest->mpArray=new CArray;
		CopyArray(p_dest->mpArray,p_source->mpArray);
		break;

	case ESYMBOLTYPE_QSCRIPT:
	{
		Dbg_MsgAssert(p_source->mpScript,("NULL p_source->mpScript ?"));
		
		// Allocate a buffer off the script heap
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
		Dbg_MsgAssert(p_source->mScriptSize,("Zero source script size"));
		p_dest->mpScript=(uint8*)Mem::Malloc(p_source->mScriptSize);
		p_dest->mScriptSize=p_source->mScriptSize;
		Mem::Manager::sHandle().PopContext();
		
		// Copy the script into the new buffer.
		const uint8 *p_from=p_source->mpScript;
		uint8 *p_to=p_dest->mpScript;
		for (uint32 i=0; i<p_source->mScriptSize; ++i)
		{
			*p_to++=*p_from++;
		}
		
		break;
	}
		
	default:
		Dbg_MsgAssert(0,("Bad p_source->mType of '%s'",GetTypeName(p_source->mType)));
		break;
	}
}

// This will run through all the elements of p_array deleting any entities pointed to by it.
// It will also call the array's Clear function to delete the array buffer, so it is not
// necessary to call Clear again after calling this.
void CleanUpArray(CArray *p_array)
{
	Dbg_MsgAssert(p_array,("NULL p_array"));
	
	ESymbolType type=p_array->GetType();
	uint32 size=p_array->GetSize();
	uint32 *p_array_data=p_array->GetArrayPointer();
	if (size)
	{
		Dbg_MsgAssert(p_array_data,("NULL p_array_data ?"));
	}	
	
	switch (type)
	{
		case ESYMBOLTYPE_NONE:
		case ESYMBOLTYPE_INTEGER:
		case ESYMBOLTYPE_FLOAT:
		case ESYMBOLTYPE_NAME:
			// Nothing to delete
			break;
			
		case ESYMBOLTYPE_STRING:
		case ESYMBOLTYPE_LOCALSTRING:
		{
			char **pp_string=(char**)p_array_data;
			for (uint32 i=0; i<size; ++i)
			{
				if (*pp_string)
				{
					DeleteString(*pp_string);
					*pp_string=NULL;
				}
				++pp_string;	
			}
			break;
		}
					
		case ESYMBOLTYPE_PAIR:
		{
			CPair **pp_pair=(CPair**)p_array_data;
			for (uint32 i=0; i<size; ++i)
			{
				if (*pp_pair)
				{
					delete *pp_pair;
					*pp_pair=NULL;
				}
				++pp_pair;	
			}
			break;
		}
			
		case ESYMBOLTYPE_VECTOR:
		{
			CVector **pp_vector=(CVector**)p_array_data;
			for (uint32 i=0; i<size; ++i)
			{
				if (*pp_vector)
				{
					delete *pp_vector;
					*pp_vector=NULL;
				}
				++pp_vector;	
			}
			break;
		}
			
		case ESYMBOLTYPE_STRUCTURE:
		{
			CStruct **pp_structure=(CStruct**)p_array_data;
			for (uint32 i=0; i<size; ++i)
			{
				if (*pp_structure)
				{
					delete *pp_structure;
					*pp_structure=NULL;
				}
				++pp_structure;	
			}
			break;
		}
			
		case ESYMBOLTYPE_ARRAY:
		{
			CArray **pp_array=(CArray**)p_array_data;
			for (uint32 i=0; i<size; ++i)
			{
				if (*pp_array)
				{
					CleanUpArray(*pp_array);
					delete *pp_array;
					*pp_array=NULL;
				}
				++pp_array;	
			}
			break;
		}
		
		default:
			Dbg_MsgAssert(0,("Bad CArray::m_type of '%s'",GetTypeName(type)));
			break;
	}		
	
	// This will delete the actual array buffer too.
	p_array->Clear();
}

// Copies the contents of p_source into p_dest. No pointers will be copied over, new entities will be
// created for p_dest. So p_source can safely be deleted afterwards.
void CopyArray(CArray *p_dest, const CArray *p_source)
{
	Dbg_MsgAssert(p_source,("NULL p_source ?"));
	Dbg_MsgAssert(p_dest,("NULL p_dest ?"));
	
	// Make sure that p_dest is cleaned up first.
	CleanUpArray(p_dest);
		
	ESymbolType type=p_source->GetType();
	uint32 size=p_source->GetSize();
	
	p_dest->SetSizeAndType(size,type);

	if (size==0)
	{
		// Finished.
		return;
	}
		
	uint32 *p_source_array=p_source->GetArrayPointer();
	Dbg_MsgAssert(p_source_array,("NULL p_source_array ?"));
	
	uint32 *p_dest_array=p_dest->GetArrayPointer();
	Dbg_MsgAssert(p_dest_array,("NULL p_dest_array ?"));
			
	switch (type)
	{
		case ESYMBOLTYPE_INTEGER:
		case ESYMBOLTYPE_FLOAT:
		case ESYMBOLTYPE_NAME:
		{
			uint32 *p_source_word=p_source_array;
			uint32 *p_dest_word=p_dest_array;
			for (uint32 i=0; i<size; ++i)
			{
				*p_dest_word++=*p_source_word++;
			}	
			break;
		}	
			
		case ESYMBOLTYPE_STRING:
		case ESYMBOLTYPE_LOCALSTRING:
		{
			char **pp_source_string=(char**)p_source_array;
			char **pp_dest_string=(char**)p_dest_array;
			for (uint32 i=0; i<size; ++i)
			{
				char *p_source_string=*pp_source_string;
				if (p_source_string)
				{
					// Create a new copy of the source string.
					*pp_dest_string=CreateString(p_source_string);
				}	
				++pp_source_string;
				++pp_dest_string;
			}	
			break;
		}	
			
		case ESYMBOLTYPE_PAIR:
		{
			CPair **pp_source_pair=(CPair**)p_source_array;
			CPair **pp_dest_pair=(CPair**)p_dest_array;
			for (uint32 i=0; i<size; ++i)
			{
				CPair *p_source_pair=*pp_source_pair;
				if (p_source_pair)
				{
					CPair *p_new_pair=new CPair;
					// The default assignement operator works OK for CPair since it contains no pointers.
					*p_new_pair=*p_source_pair;
					*pp_dest_pair=p_new_pair;
				}
				++pp_source_pair;
				++pp_dest_pair;
			}		
			break;
		}
					
		case ESYMBOLTYPE_VECTOR:
		{
			CVector **pp_source_vector=(CVector**)p_source_array;
			CVector **pp_dest_vector=(CVector**)p_dest_array;
			for (uint32 i=0; i<size; ++i)
			{
				CVector *p_source_vector=*pp_source_vector;
				if (p_source_vector)
				{
					CVector *p_new_vector=new CVector;
					// The default assignement operator works OK for CVector since it contains no pointers.
					*p_new_vector=*p_source_vector;
					*pp_dest_vector=p_new_vector;
				}
				++pp_source_vector;
				++pp_dest_vector;
			}		
			break;
		}
			
		case ESYMBOLTYPE_STRUCTURE:
		{
			CStruct **pp_source_structure=(CStruct**)p_source_array;
			CStruct **pp_dest_structure=(CStruct**)p_dest_array;
			for (uint32 i=0; i<size; ++i)
			{
				CStruct *p_source_structure=*pp_source_structure;
				if (p_source_structure)
				{
					CStruct *p_new_structure=new CStruct;
					// Copy the contents using CStruct's assignement operator.
					*p_new_structure=*p_source_structure;
					*pp_dest_structure=p_new_structure;
				}
				++pp_source_structure;
				++pp_dest_structure;
			}		
			break;
		}
			
		case ESYMBOLTYPE_ARRAY:
		{
			CArray **pp_source_carray=(CArray**)p_source_array;
			CArray **pp_dest_carray=(CArray**)p_dest_array;
			for (uint32 i=0; i<size; ++i)
			{
				CArray *p_source_carray=*pp_source_carray;
				if (p_source_carray)
				{
					CArray *p_new_carray=new CArray;
					// No assignement operator defined for CArray (cos it would cause cyclic dependencies) 
					// so need to use CopyArray.
					CopyArray(p_new_carray,p_source_carray);
					*pp_dest_carray=p_new_carray;
				}
				++pp_source_carray;
				++pp_dest_carray;
			}
			break;
		}
		
		default:
			Dbg_MsgAssert(0,("Bad source array type of '%s'",GetTypeName(type)));
			break;
	}
}



// Initialises all the members.
void CStruct::init()
{
	mp_components=NULL;
	
	#ifdef __NOPT_ASSERT__ 
	mp_parent_script=NULL;
	#endif
	
#ifdef	__DEBUG_STRUCT_ALLOCS
	if (init_last)
	{
		for (int i=0;i<MAX_LAST;i++)
		{
			last[i] = NULL;
		}
		head = 0;
		init_last = 0;
	}
	
	last[head++] = this; 
	if (head == MAX_LAST)
		head = 0;
#endif	
}




static int x = 0;
static int max = 18000;
	
// Usual constructor.	
CStruct::CStruct()
{
	// Initialise everything. CStruct is not derived from CClass so we don't get
	// the auro-zeroing.
	init();

	#if 0
	// For tracking down Struct leaks
	// when we go over a certain maximum, start printing out the callstack of the structs that take us over that max	
	if (CStruct::SGetNumUsedItems()>max)
	{
		max = CStruct::SGetNumUsedItems();
		DumpUnwindStack(20,0);
	}
	#endif
}

// Copy constructor.
CStruct::CStruct( const CStruct& rhs )
{
	// Initialise everything. CStruct is not derived from CClass so we don't get
	// the auro-zeroing.
	init();

	// use the overridden assignment operator
	*this = rhs;
	
	x++;
	if (x>max)
	{
		max = x;
//		DumpUnwindStack(20,0);
	}
}

// Assignement operator.
CStruct& CStruct::operator=( const CStruct& rhs )
{
	// don't try to assign to yourself
	if ( &rhs == this )
	{
		return *this;
	}

	this->Clear();
	*this+=rhs;

	return *this;
}

// This will merge the contents of the rhs into this structure.
// Functionally the same as the old AppendStructure function, except AppendStructure would accept a NULL pointer.
CStruct& CStruct::operator+=( const CStruct& rhs )
{
	CComponent *p_source_component=rhs.mp_components;
	while (p_source_component)
	{
		CComponent *p_new=new CComponent;
		CopyComponent(p_new, p_source_component);
		AddComponent(p_new);
		p_source_component=p_source_component->mpNext;
	}	
	return *this;
}

// TODO: Remove at some point. Provided for back-compatibility.
void CStruct::AppendStructure(const CStruct *p_struct)
{
	if (p_struct)
	{
		*this+=*p_struct;
	}	
}

// Written specially for use by CScriptDebugger::transmit_cscript_list.
// This will append the passed component to the structure's list of components
// and will leave the existing list untouched, ie it will not remove any of the same name.
// This is so that the script debugger code can store a list of CScript instances into a structure
// even though many may have the same name.
void CStruct::AppendComponentPointer(CComponent *p_comp)
{
	CComponent *p_last=NULL;
	CComponent *p_scan=mp_components;
	
	// Find p_last, the last component in the list.
	while (p_scan)
	{
		p_last=p_scan;
		p_scan=p_scan->mpNext;
	}		
	
	// Tag p_comp onto the end of the list.
	if (p_last)
	{
		p_last->mpNext=p_comp;
	}
	else
	{
		mp_components=p_comp;
	}	
	p_comp->mpNext=NULL;
}

CStruct::~CStruct()
{

#ifdef	__DEBUG_STRUCT_ALLOCS
	for (int i=0;i<MAX_LAST;i++)
	{
		if (last[i] == this)
		{
			// step the head back to the last entry added
			head--;
			if (head <0)
			{
				head = MAX_LAST-1;
			}
			// if the head is not the entry we just removed 
			// then copy the head into this slot
			if (i!=head)
			{
				last[i] = last[head];
			}
			// and clear the head, which now contains the entry we deleted
			last[head] = NULL;
			// and we don't need to iterate no more
			break;
		}
	}
#endif	
	Clear();
}

// Deletes all components, deleting any arrays or structures referenced by the components.
void CStruct::Clear()
{
	CComponent *p_comp=mp_components;
	while (p_comp)
	{
		CComponent *p_next=p_comp->mpNext;
		// Note: The CComponent destructor cannot clean up, because that would cause cyclic dependencies.
		CleanUpComponent(p_comp);
		delete p_comp;
		p_comp=p_next;
	}
	mp_components=NULL;
}

void CStruct::RemoveComponent(uint32 nameChecksum)
{
	CComponent *p_last=NULL;
	CComponent *p_comp=mp_components;
	while (p_comp)
	{
		CComponent *p_next=p_comp->mpNext;
		
		if (p_comp->mNameChecksum==nameChecksum)
		{
			// p_comp must be removed.
			
			// Unlink it.
			if (p_last)
			{
				p_last->mpNext=p_next;
			}
			else	
			{
				Dbg_MsgAssert(p_comp==mp_components,("Eh ? p_comp!=mp_components ??"));
				mp_components=p_next;
			}
			
			// Note: The CComponent destructor cannot clean up, because that would cause cyclic dependencies.
			CleanUpComponent(p_comp);
			delete p_comp;
			
			// Carries on, in case there is more than one component with the given name.
			p_comp=p_next;
		}	
		else
		{
			p_last=p_comp;
			p_comp=p_next;
		}	
	}	
}

void CStruct::RemoveComponent(const char *p_name)
{
	RemoveComponent(Crc::GenerateCRCFromString(p_name));
}

// Same as RemoveComponent except the type must matched the passed type too.
// Used by eval.cpp when subtracting a structure from another structure.
void CStruct::RemoveComponentWithType(uint32 nameChecksum, uint8 type)
{
	CComponent *p_last=NULL;
	CComponent *p_comp=mp_components;
	while (p_comp)
	{
		CComponent *p_next=p_comp->mpNext;
		
		if (p_comp->mNameChecksum==nameChecksum && p_comp->mType==type)
		{
			// p_comp must be removed.
			
			// Unlink it.
			if (p_last)
			{
				p_last->mpNext=p_next;
			}
			else	
			{
				Dbg_MsgAssert(p_comp==mp_components,("Eh ? p_comp!=mp_components ??"));
				mp_components=p_next;
			}
			
			// Note: The CComponent destructor cannot clean up, because that would cause cyclic dependencies.
			CleanUpComponent(p_comp);
			delete p_comp;
			
			// Carries on, in case there is more than one component with the given name.
			p_comp=p_next;
		}	
		else
		{
			p_last=p_comp;
			p_comp=p_next;
		}	
	}	
}

void CStruct::RemoveComponentWithType(const char *p_name, uint8 type)
{
	RemoveComponentWithType(Crc::GenerateCRCFromString(p_name),type);
}

// Note: Logical inconsistencies that could be an issue later maybe:
// If the unnamed-name resolves to a global structure, should the component still be removed?
// If there is a referenced global structure, should it have any of the specified flag removed
// too, and so on recursively? (probably not, since global structures should remain constant)
void CStruct::RemoveFlag(uint32 checksum)
{
	CComponent *p_last=NULL;
	CComponent *p_comp=mp_components;
	while (p_comp)
	{
		CComponent *p_next=p_comp->mpNext;
		
		if (p_comp->mNameChecksum==0 && p_comp->mChecksum==checksum)
		{
			// p_comp must be removed.
			
			// Unlink it.
			if (p_last)
			{
				p_last->mpNext=p_next;
			}
			else	
			{
				Dbg_MsgAssert(p_comp==mp_components,("Eh ? p_comp!=mp_components ??"));
				mp_components=p_next;
			}
			
			// Note: The CComponent destructor cannot clean up, because that would cause cyclic dependencies.
			CleanUpComponent(p_comp);
			// Note: All the CleanUpComponent call will have done will be to zero the union,
			// cos a flag component does not contain any pointers that need deleting.
			// So I could have just set p_comp->mChecksum to 0 instead. Just calling
			// CleanUpComponent for consistency.
			delete p_comp;
			
			// Carries on, in case there is more than one flag with the given name.
			// There shouldn't be, but check anyway.
			p_comp=p_next;
		}	
		else
		{
			p_last=p_comp;
			p_comp=p_next;
		}	
	}	
}

void CStruct::RemoveFlag(const char *p_name)
{
	RemoveFlag(Crc::GenerateCRCFromString(p_name));
}

// Returns true if the structure contains no components.
bool CStruct::IsEmpty() const
{
	return mp_components==NULL;
}

// Searches for a component with the given name, but will also recurse into substructures.
// Used when resolving the <ParamName> syntax.
CComponent *CStruct::FindNamedComponentRecurse(uint32 nameChecksum) const
{
	CComponent *p_found=NULL;
	
    CComponent *p_comp=mp_components;
    while (p_comp)
    {
        if (p_comp->mNameChecksum==nameChecksum) 
		{
			p_found=p_comp;
		}	
		else if (p_comp->mNameChecksum==0 && p_comp->mType==ESYMBOLTYPE_NAME)
		{
            CSymbolTableEntry *p_entry=Resolve(p_comp->mChecksum);
            if (p_entry && p_entry->mType==ESYMBOLTYPE_STRUCTURE)
            {
                Dbg_MsgAssert(p_entry->mpStructure,("NULL p_entry->mpStructure"));
                CComponent *p_new_found=p_entry->mpStructure->FindNamedComponentRecurse(nameChecksum);
				if (p_new_found)
				{
					p_found=p_new_found;
				}	
            }
		}
        p_comp=p_comp->mpNext;
    }
	
    return p_found;
}

// If passed NULL this will return the first (leftmost) component.
// If passed non-NULL it return the next component (to the right) 
// Returns NULL if the passed component is the last component.
CComponent *CStruct::GetNextComponent(CComponent *p_comp) const
{
	if (p_comp==NULL)
	{
		return mp_components;
	}
	
	return p_comp->mpNext;	
}

// This will copy the contents of this structure into p_dest, but in such a way that
// p_dest will contain no unnamed-structure references, they will all get expanded.
// This was added because sometimes one wants to step through each of the components of
// a structure using GetNextComponent, but GetNextComponent itself cannot recurse into
// unnamed structures, because when it reaches the end of one it won't know how to get back
// to the parent.
// So instead just create a new structure, copy the other structure into it using ExpandInto,
// then step through that using GetNextComponent, then delete the new structure once finished.
// It's a bit memory intensive, but saves having to write special code to resolve unnamed
// structures & recursing whenever one wants to scan through the components.
// Could be handy for other things later too.
// recursionCount is just to catch infinite recursion, so no need to pass it a value. (defaults to 0)
void CStruct::ExpandInto(CStruct *p_dest, int recursionCount) const
{
	Dbg_MsgAssert(recursionCount<=20,("Possible infinite recursion of CStruct::ExpandInto! More than 20 recursions have occurred."));
	Dbg_MsgAssert(p_dest,("NULL p_dest sent to ExpandInto"));
	
	CComponent *p_comp=mp_components;
	while (p_comp)
	{
		bool added_structure=false;
		
		if (p_comp->mNameChecksum==0)
		{
			Dbg_MsgAssert(p_comp->mType!=ESYMBOLTYPE_STRUCTURE,("Unexpected unnamed structure in CStruct"));
			
			if (p_comp->mType==ESYMBOLTYPE_NAME)
			{
				CSymbolTableEntry *p_global=Resolve(p_comp->mChecksum);
				if (p_global && p_global->mType==ESYMBOLTYPE_STRUCTURE)
				{
					Dbg_MsgAssert(p_global->mpStructure,("NULL p_global->mpStructure"));
					p_global->mpStructure->ExpandInto(p_dest,recursionCount+1);
					added_structure=true;
				}	
			}	
		}
		
		if (!added_structure)
		{
			CComponent *p_new_component=new CComponent;
			CopyComponent(p_new_component,p_comp);
			p_dest->AddComponent(p_new_component);
		}
		
		p_comp=p_comp->mpNext;
	}
}

// Adds the component p_comp to the end of the list of components, removing any existing components
// that have the same name and type, and asserting if any components exist with the same name but different
// type.
// However, if p_comp is a flag (an un-named component of type ESYMBOLTYPE_NAME) then it will always add it,
// because otherwise we would not be able to have more than one flag in a structure.
//
// Note: Even two identical flags are allowed, because sometimes this is handy. For example I use a list
// of words in a structure as a way for Scott to represent trick button sequences (see airtricks.q) and 
// sometimes two of the words might need to be identical.
//
// Another note: This function needs to be pretty fast because it will need to be called many times for
// each line of script executed. Optimize later.
//
void CStruct::AddComponent(CComponent *p_comp)
{
	Dbg_MsgAssert(p_comp,("NULL p_comp"));

	// Unnamed structure components are not allowed, because to support them would require 
	// modifying the search_for function. No need, since when structures are created from lists
	// of tokens any substructures get expanded and added component by component at that stage.
	// So in theory no components of type structure should get added.
	Dbg_MsgAssert(!(p_comp->mNameChecksum==0 && p_comp->mType==ESYMBOLTYPE_STRUCTURE),("Tried to add an un-named structure component ..."));

	Dbg_MsgAssert(p_comp->mType!=ESYMBOLTYPE_NONE,("Tried to add a structure component with no type, name='%s' ...",FindChecksumName(p_comp->mNameChecksum)));
	
	CComponent *p_last=NULL;
	CComponent *p_scan=mp_components;
    bool remove=false;
	
	while (p_scan)
	{
		if (p_scan->mNameChecksum==p_comp->mNameChecksum)
		{
			// Same name ...
		 
		    remove=false;
			
			if (p_scan->mType==p_comp->mType)
			{
				if (p_scan->mType==ESYMBOLTYPE_NAME && 
					p_scan->mNameChecksum==0 && 
					p_scan->mChecksum!=p_comp->mChecksum)
				{
					// Allow multiple un-named checksums if their values are different, because
					// these are flags.
				}
				else
				{
					remove=true;
				}
			}
			else
			{
				// Consider floats and ints to be the same type, so that x=3.1 overrides x=3 and vice versa				
				if ((p_scan->mType==ESYMBOLTYPE_FLOAT && p_comp->mType==ESYMBOLTYPE_INTEGER) ||
					(p_scan->mType==ESYMBOLTYPE_INTEGER && p_comp->mType==ESYMBOLTYPE_FLOAT))
				{
					remove=true;
				}
			}
			
			if (remove)		
			{
				// Remove this component.
				if (p_last)
				{
					p_last->mpNext=p_scan->mpNext;
					CleanUpComponent(p_scan);
					delete p_scan;
					p_scan=p_last->mpNext;
				}	
				else
				{
					mp_components=p_scan->mpNext;
					CleanUpComponent(p_scan);
					delete p_scan;
					p_scan=mp_components;
				}	
			}
			else
			{
				// Same name but different type.
				// The old code would assert in this case, but the new code allows it.
				// This is because there is no problem if a structure contains {x=7 x="Blaa"}
				// Getting an integer called x from it would give 7, getting a string called x would give "Blaa"
				// Also, it would allow {x=7 x=foo} where foo may be defined elsewhere to be a different integer,
				// which is useful.
				p_last=p_scan;
				p_scan=p_scan->mpNext;
			}	
		}	
		else
		{
			p_last=p_scan;
			p_scan=p_scan->mpNext;
		}	
	}	
	
	// Now, p_last points to the last component in the list, and may be NULL.
	// Tag p_comp onto the end of the list.
	if (p_last)
	{
		p_last->mpNext=p_comp;
	}
	else
	{
		mp_components=p_comp;
	}	
	p_comp->mpNext=NULL;
}

#ifdef __NOPT_ASSERT__ 
void CStruct::SetParentScript(CScript *p_parentScript)
{
	mp_parent_script=p_parentScript;
}

CScript *CStruct::GetParentScript() const
{
	return mp_parent_script;
}
#endif


void CStruct::AddString(uint32 nameChecksum, const char *p_string)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_STRING;
	p_new->mpString=CreateString(p_string);
	
	AddComponent(p_new);
}

void CStruct::AddString(const char *p_name, const char *p_string)
{
	AddString(Crc::GenerateCRCFromString(p_name),p_string);
}

void CStruct::AddLocalString(uint32 nameChecksum, const char *p_string)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_LOCALSTRING;
	p_new->mpString=CreateString(p_string);
	
	AddComponent(p_new);
}

void CStruct::AddLocalString(const char *p_name, const char *p_string)
{
	AddLocalString(Crc::GenerateCRCFromString(p_name),p_string);
}

void CStruct::AddInteger(uint32 nameChecksum, int integer)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_INTEGER;
	p_new->mIntegerValue=integer;
	
	AddComponent(p_new);
}

void CStruct::AddInteger(const char *p_name, int integer)
{
	AddInteger(Crc::GenerateCRCFromString(p_name),integer);
}

void CStruct::AddFloat(uint32 nameChecksum, float float_val)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_FLOAT;
	p_new->mFloatValue=float_val;
	
	AddComponent(p_new);
}

void CStruct::AddFloat(const char *p_name, float float_val)
{
	AddFloat(Crc::GenerateCRCFromString(p_name),float_val);
}

void CStruct::AddChecksum(uint32 nameChecksum, uint32 checksum)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_NAME;
	p_new->mChecksum=checksum;
	
	AddComponent(p_new);
}

void CStruct::AddChecksum(const char *p_name, uint32 checksum)
{
	AddChecksum(Crc::GenerateCRCFromString(p_name),checksum);
}

void CStruct::AddVector(uint32 nameChecksum, float x, float y, float z)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_VECTOR;
	p_new->mpVector=new CVector;
	p_new->mpVector->mX=x;
	p_new->mpVector->mY=y;
	p_new->mpVector->mZ=z;
	
	AddComponent(p_new);
}

void CStruct::AddVector(const char *p_name, float x, float y, float z)
{
	AddVector(Crc::GenerateCRCFromString(p_name),x,y,z);
}

void CStruct::AddVector( uint32 nameChecksum, Mth::Vector vector )
{
	AddVector( nameChecksum, vector.GetX(), vector.GetY(), vector.GetZ() );
}

void CStruct::AddVector( const char* p_name, Mth::Vector vector )
{
	AddVector( Crc::GenerateCRCFromString( p_name ), vector.GetX(), vector.GetY(), vector.GetZ() );
}

void CStruct::AddPair(uint32 nameChecksum, float x, float y)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_PAIR;
	p_new->mpPair=new CPair;
	p_new->mpPair->mX=x;
	p_new->mpPair->mY=y;
	
	AddComponent(p_new);
}

void CStruct::AddPair(const char *p_name, float x, float y)
{
	AddPair(Crc::GenerateCRCFromString(p_name),x,y);
}

// Creates a new array & copies in the contents of the passed array.
void CStruct::AddArray(uint32 nameChecksum, const CArray *p_array)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_ARRAY;
	p_new->mpArray=new CArray;
	CopyArray(p_new->mpArray,p_array);

	AddComponent(p_new);
}

void CStruct::AddArray(const char *p_name, const CArray *p_array)
{
	AddArray(Crc::GenerateCRCFromString(p_name),p_array);
}

// This stores the passed pointer rather than copying the contents, so in this case it
// is important for the calling code not to delete the passed pointer.
// The CStruct will delete it in its destructor.
// Faster than copying the contents.
// Used parse.cpp, when creating the structure for holding function params.
void CStruct::AddArrayPointer(uint32 nameChecksum, CArray *p_array)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_ARRAY;
	p_new->mpArray=p_array;

	AddComponent(p_new);
}

void CStruct::AddArrayPointer(const char *p_name, CArray *p_array)
{
	AddArrayPointer(Crc::GenerateCRCFromString(p_name),p_array);
}

// Creates a new structure & copies in the contents of the passed structure.
void CStruct::AddStructure(uint32 nameChecksum, const CStruct *p_structure)
{
	Dbg_MsgAssert(p_structure,("NULL p_structure"));
	
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_STRUCTURE;
	p_new->mpStructure=new CStruct;
	*p_new->mpStructure=*p_structure;

	AddComponent(p_new);
}

void CStruct::AddStructure(const char *p_name, const CStruct *p_structure)
{
	AddStructure(Crc::GenerateCRCFromString(p_name),p_structure);
}

// This stores the passed pointer rather than copying the contents, so in this case it
// is important for the calling code not to delete the passed pointer.
// The CStruct will delete it in its destructor.
// Faster than copying the contents.
// Used parse.cpp, when creating the structure for holding function params.
void CStruct::AddStructurePointer(uint32 nameChecksum, CStruct *p_structure)
{
	Dbg_MsgAssert(p_structure,("NULL p_structure"));
	
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_STRUCTURE;
	p_new->mpStructure=p_structure;

	AddComponent(p_new);
}

void CStruct::AddStructurePointer(const char *p_name, CStruct *p_structure)
{
	AddStructurePointer(Crc::GenerateCRCFromString(p_name),p_structure);
}

void CStruct::AddScript(uint32 nameChecksum, const uint8 *p_scriptTokens, uint32 size)
{
	CComponent *p_new=new CComponent;
	
	p_new->mNameChecksum=nameChecksum;
	p_new->mType=ESYMBOLTYPE_QSCRIPT;
	p_new->mScriptSize=size;
	
	// Allocate a buffer off the script heap
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	Dbg_MsgAssert(size,("Zero script size"));
	uint8 *p_new_script=(uint8*)Mem::Malloc(size);
	Mem::Manager::sHandle().PopContext();
	
	// Copy the script into the new buffer.
	Dbg_MsgAssert(p_scriptTokens,("NULL p_scriptTokens"));
	const uint8 *p_source=p_scriptTokens;
	uint8 *p_dest=p_new_script;
	for (uint32 i=0; i<size; ++i)
	{
		*p_dest++=*p_source++;
	}	
	
	// Give the new buffer to the new component.
	p_new->mpScript=p_new_script;
		
	AddComponent(p_new);
}

void CStruct::AddScript(const char *p_name, const uint8 *p_scriptTokens, uint32 size)
{
	AddScript(Crc::GenerateCRCFromString(p_name),p_scriptTokens,size);
}

///////////////////////////////////////////////////////////////////////////////////
// TODO: Remove all these AddComponent functions at some point.
// They are only included to provide back compatibility with the old code without
// having to change thousands of occurrences of calls to AddComponent.
// Gradually phase out the old AddComponent instead.
//

// String or local string.
void CStruct::AddComponent(uint32 nameChecksum, ESymbolType type, const char *p_string)
{
    Dbg_MsgAssert(type==ESYMBOLTYPE_LOCALSTRING || type==ESYMBOLTYPE_STRING,("Bad type sent to AddComponent"));
    Dbg_MsgAssert(p_string,("NULL p_string"));

	if (type==ESYMBOLTYPE_STRING)
	{
		AddString(nameChecksum,p_string);
	}
	else	
	{
		AddLocalString(nameChecksum,p_string);
	}
}

// Integer or any other 4 byte value.
void CStruct::AddComponent(uint32 nameChecksum, ESymbolType type, int integer)
{
	switch (type)
	{
		case ESYMBOLTYPE_INTEGER: 
			AddInteger(nameChecksum,integer);
			break;
		case ESYMBOLTYPE_FLOAT:
			AddFloat(nameChecksum,*(float*)&integer);
			break;
		case ESYMBOLTYPE_STRUCTUREPOINTER:
			AddStructurePointer(nameChecksum,(CStruct*)integer);
			break;
		case ESYMBOLTYPE_ARRAY:
			AddArrayPointer(nameChecksum,(CArray*)integer);
			break;
		case ESYMBOLTYPE_NAME:
			AddChecksum(nameChecksum,(uint32)integer);
			break;
		default:	
			Dbg_MsgAssert(0,("Bad type of '%s' sent to AddComponent",GetTypeName(type)));
			break;
	}		
}

// Vector
void CStruct::AddComponent(uint32 nameChecksum, float x, float y, float z)
{
	AddVector(nameChecksum,x,y,z);
}

// Pair
void CStruct::AddComponent(uint32 nameChecksum, float x, float y)
{
	AddPair(nameChecksum,x,y);
}

// Array
void CStruct::AddComponent(uint32 nameChecksum, CArray *p_array)
{
	AddArray(nameChecksum,p_array);
}

///////////////////////////////////////////////////////////////////////////////////


struct SWhatever
{
	union
	{
		int mIntegerValue;
		float mFloatValue;
		char *mpString;
		char *mpLocalString;
		CPair *mpPair;
		CVector *mpVector;
		CStruct *mpStructure;
		CArray *mpArray;
		uint8 *mpScript;
		uint32 mChecksum;
		uint32 mUnion; 
	};
};

// Infinite recursion of search_for could occur, for example if we have a global structure foo={foo}
// which is trying to include itself.
// This counter will make an assert go off if search_for tries to recurse a suspicious number of times.
#ifdef __NOPT_ASSERT__ 
static uint32 s_num_search_for_recursions=0;
#endif

bool CStruct::search_for(uint32 nameChecksum, ESymbolType type, SWhatever *p_value) const
{
	Dbg_MsgAssert(s_num_search_for_recursions<10,("Possible infinite recursion of the CStruct::search_for function, bailing out!\nLast search was for parameter named '%s' of type '%s'",FindChecksumName(nameChecksum),GetTypeName(type)));
	#ifdef __NOPT_ASSERT__
	++s_num_search_for_recursions;
	#endif
	
	Dbg_MsgAssert(p_value,("NULL p_value"));
	
	bool found=false;
	
	CComponent *p_comp=mp_components;
	while (p_comp)
	{
		if (p_comp->mNameChecksum==nameChecksum)
		{
			// The name of the component matches that required.
			if (p_comp->mType==type)
			{
				// The type also matches, so the required component has been found.
				// Only set a flag though. Need to keep searching in case the structure
				// has the form {x=7 x=foo} where foo is a global defined to be an integer,
				// and which must override the previous 7.
				
				p_value->mUnion=p_comp->mUnion;
				found=true;
			}
			else
			{
				// The type does not match.
				if (p_comp->mType==ESYMBOLTYPE_NAME)
				{
					// The name of the component matches what we're looking for, but the type
					// does not and is of type name.
					// For example, we might be searching for an integer called x,
					// but we've found a component x=foo
					
					// So, see if foo is the name of a global defined somewhere.
					CSymbolTableEntry *p_global=Resolve(p_comp->mChecksum);
					if (p_global)
					{
						// It is! So check if this global has the type that we're looking for ...
						if (p_global->mType==type)
						{
							// It does! So we have a match.
							// Just set the found flag though, because we need to keep searching in
							// case the structure has the form {x=foo x=7} where the later 7 needs to
							// override what we've just found.
							p_value->mUnion=p_global->mUnion;
							found=true;
						}
					}
				}
			}				
		}
		else
		{
			// The name of the component does not match what we're looking for.
			
			// See if the component is an un-named name, eg the blaa in {a=3 blaa}
			if (p_comp->mNameChecksum==0 && p_comp->mType==ESYMBOLTYPE_NAME)
			{
				// It is, so check whether it resolves to a global structure.
				// If a global structure is referenced by name in another structure, then
				// it is considered 'pasted in' to that structure, so it needs to be searched too.
				CSymbolTableEntry *p_global=Resolve(p_comp->mChecksum);
				if (p_global)
				{
					// It is a global something ...
					if (p_global->mType==ESYMBOLTYPE_STRUCTURE)
					{
						// It is a structure. So search it for the required component also.
						Dbg_MsgAssert(p_global->mpStructure,("NULL p_global->mpStructure ?"));
						SWhatever value;
						if (p_global->mpStructure->search_for(nameChecksum,type,&value))
						{
							// Found! Load the value, set the found flag and carry on.
							*p_value=value;
							found=true;
						}
					}
				}			
			}	
		}
			
		p_comp=p_comp->mpNext;
	}	

	Dbg_MsgAssert(s_num_search_for_recursions,("Eh ???"));
	#ifdef __NOPT_ASSERT__
	--s_num_search_for_recursions;
	#endif
	
	return found;
}


bool CStruct::GetString(uint32 nameChecksum, const char **pp_text, EAssertType assert) const
{
	Dbg_MsgAssert(pp_text,("NULL pp_text"));
	
	SWhatever value;
	if (search_for(nameChecksum,ESYMBOLTYPE_STRING,&value))
	{
		*pp_text=value.mpString;
		return true;
	}
	if (search_for(nameChecksum,ESYMBOLTYPE_LOCALSTRING,&value))
	{
		*pp_text=value.mpString;
		return true;
	}
	if (assert)
	{
		PrintContents(this);
		Dbg_MsgAssert(0,("Could not find string component named '%s' in structure",FindChecksumName(nameChecksum)));
	}	
	return false;	
}

bool CStruct::GetString(const char *p_paramName, const char **pp_text, EAssertType assert) const
{
	return GetString(Crc::GenerateCRCFromString(p_paramName),pp_text,assert);
}

bool CStruct::GetLocalString(uint32 nameChecksum, const char **pp_text, EAssertType assert) const
{
	return GetString(nameChecksum,pp_text,assert);
}

bool CStruct::GetLocalString(const char *p_paramName, const char **pp_text, EAssertType assert) const
{
	return GetString(Crc::GenerateCRCFromString(p_paramName),pp_text,assert);
}

bool CStruct::GetInteger(uint32 nameChecksum, int *p_integerValue, EAssertType assert) const
{
	Dbg_MsgAssert(p_integerValue,("NULL p_integerValue"));
	
	SWhatever value;
	if (search_for(nameChecksum,ESYMBOLTYPE_INTEGER,&value))
	{
		*p_integerValue=value.mIntegerValue;
		return true;
	}
	if (assert)
	{
		PrintContents(this);
		Dbg_MsgAssert(0,("Could not find integer component named '%s' in structure",FindChecksumName(nameChecksum)));
	}	
	return false;	
}

bool CStruct::GetInteger(const char *p_paramName, int *p_integerValue, EAssertType assert) const
{
	return GetInteger(Crc::GenerateCRCFromString(p_paramName),p_integerValue,assert);
}

bool CStruct::GetFloat(uint32 nameChecksum, float *p_floatValue, EAssertType assert) const
{
	Dbg_MsgAssert(p_floatValue,("NULL p_floatValue"));
	
	SWhatever value;
	if (search_for(nameChecksum,ESYMBOLTYPE_FLOAT,&value))
	{
		*p_floatValue=value.mFloatValue;
		return true;
	}
	// If a float was not found, check for any int's with the same name, and if found cast to a float.
	// Note that the extra search means that {x=3.0} will result in x being found quicker than {x=3}
	// Could speed this up by making search_for() match integers with floats, but that would slightly slow
	// down the other Get... functions. Need to do some tests to see what is best.
	if (search_for(nameChecksum,ESYMBOLTYPE_INTEGER,&value))
	{
		*p_floatValue=value.mIntegerValue;
		return true;
	}
	if (assert)
	{
		PrintContents(this);
		Dbg_MsgAssert(0,("Could not find float component named '%s' in structure",FindChecksumName(nameChecksum)));
	}	
	return false;	
}

bool CStruct::GetFloat(const char *p_paramName, float *p_floatValue, EAssertType assert) const
{
	return GetFloat(Crc::GenerateCRCFromString(p_paramName),p_floatValue,assert);
}

bool CStruct::GetVector(uint32 nameChecksum, Mth::Vector *p_vector, EAssertType assert) const
{
	Dbg_MsgAssert(p_vector,("NULL p_vector"));
	
	SWhatever value;
	if (search_for(nameChecksum,ESYMBOLTYPE_VECTOR,&value))
	{
		Dbg_MsgAssert(value.mpVector,("NULL value.mpVector"));
		p_vector->Set(value.mpVector->mX,value.mpVector->mY,value.mpVector->mZ);
		return true;
	}
	if (assert)
	{
		PrintContents(this);
		Dbg_MsgAssert(0,("Could not find vector component named '%s' in structure",FindChecksumName(nameChecksum)));
	}	
	return false;	
}

bool CStruct::GetVector(const char *p_paramName, Mth::Vector *p_vector, EAssertType assert) const
{
	return GetVector(Crc::GenerateCRCFromString(p_paramName),p_vector,assert);
}

bool CStruct::GetPair(uint32 nameChecksum, float *p_x, float *p_y,	EAssertType assert) const
{
	Dbg_MsgAssert(p_x,("NULL p_x"));
	Dbg_MsgAssert(p_y,("NULL p_y"));
	
	SWhatever value;
	if (search_for(nameChecksum,ESYMBOLTYPE_PAIR,&value))
	{
		Dbg_MsgAssert(value.mpPair,("NULL value.mpPair"));
		*p_x=value.mpPair->mX;
		*p_y=value.mpPair->mY;
		return true;
	}
	if (assert)
	{
		PrintContents(this);
		Dbg_MsgAssert(0,("Could not find pair component named '%s' in structure",FindChecksumName(nameChecksum)));
	}	
	return false;	
}

bool CStruct::GetPair(const char *p_paramName, float *p_x, float *p_y, EAssertType assert) const
{
	return GetPair(Crc::GenerateCRCFromString(p_paramName),p_x,p_y,assert);
}

bool CStruct::GetStructure(uint32 nameChecksum, CStruct **pp_structure,	EAssertType assert) const
{
	Dbg_MsgAssert(pp_structure,("NULL pp_structure"));
	
	SWhatever value;
	if (search_for(nameChecksum,ESYMBOLTYPE_STRUCTURE,&value))
	{
		Dbg_MsgAssert(value.mpStructure,("NULL value.mpStructure"));
		*pp_structure=value.mpStructure;
		return true;
	}
	if (assert)
	{
		PrintContents(this);
		Dbg_MsgAssert(0,("Could not find structure component named '%s' in structure",FindChecksumName(nameChecksum)));
	}	
	return false;	
}

bool CStruct::GetStructure(const char *p_paramName, CStruct **pp_structure,	EAssertType assert) const
{
	return GetStructure(Crc::GenerateCRCFromString(p_paramName),pp_structure,assert);
}

bool CStruct::GetArray(uint32 nameChecksum, CArray **pp_array, EAssertType assert) const
{
	Dbg_MsgAssert(pp_array,("NULL pp_array"));
	
	SWhatever value;
	if (search_for(nameChecksum,ESYMBOLTYPE_ARRAY,&value))
	{
		Dbg_MsgAssert(value.mpArray,("NULL value.mpArray"));
		*pp_array=value.mpArray;
		return true;
	}
	if (assert)
	{
		PrintContents(this);
		Dbg_MsgAssert(0,("Could not find array component named '%s' in structure",FindChecksumName(nameChecksum)));
	}	
	return false;	
}

bool CStruct::GetArray(const char *p_paramName, CArray **pp_array, EAssertType assert) const
{
	return GetArray(Crc::GenerateCRCFromString(p_paramName),pp_array,assert);
}

bool CStruct::GetScript(uint32 nameChecksum, SStructScript *p_structScript, EAssertType assert) const
{
	Dbg_MsgAssert(p_structScript,("NULL p_structScript"));
	
	SWhatever value;
	if (search_for(nameChecksum,ESYMBOLTYPE_QSCRIPT,&value))
	{
		Dbg_MsgAssert(value.mpScript,("NULL value.mpScript"));
		p_structScript->mNameChecksum=nameChecksum;
		p_structScript->mpScriptTokens=value.mpScript;
		return true;
	}
	if (assert)
	{
		PrintContents(this);
		Dbg_MsgAssert(0,("Could not find script component named '%s' in structure",FindChecksumName(nameChecksum)));
	}	
	return false;	
}

bool CStruct::GetScript(const char *p_paramName, SStructScript *p_structScript, EAssertType assert) const
{
	return GetScript(Crc::GenerateCRCFromString(p_paramName),p_structScript,assert);
}

bool CStruct::GetChecksum(uint32 nameChecksum, uint32 *p_checksum, EAssertType assert) const
{
	Dbg_MsgAssert(p_checksum,("NULL p_checksum"));

	// Does not use the search_for function, because GetChecksum needs to ignore unnamed names
	// that resolve to structures.
	
	bool found=false;
	CComponent *p_comp=mp_components;
	while (p_comp)
	{
		if (p_comp->mNameChecksum==nameChecksum)
		{
			if (p_comp->mType==ESYMBOLTYPE_NAME)
			{
				uint32 ch=p_comp->mChecksum;
						
				// The name matches and the type matches, but ignore any un-named names which
				// resolve to structures since these are considered 'part of' the original structure.				
				CSymbolTableEntry *p_entry=Resolve(ch);	
				if (p_entry && p_entry->mType==ESYMBOLTYPE_STRUCTURE && p_comp->mNameChecksum==0)
				{
					// Do nothing.
				}
				else
				{
					*p_checksum=ch;
					found=true;
				}	
			}
		}
		else if (p_comp->mNameChecksum==0 && p_comp->mType==ESYMBOLTYPE_NAME)
		{
			// It's an unnamed name, so check whether it is a structure.
			uint32 ch=p_comp->mChecksum;
			CSymbolTableEntry *p_entry=Resolve(ch);
			if (p_entry && p_entry->mType==ESYMBOLTYPE_STRUCTURE)
			{
				// It is a structure, so search that.
                Dbg_MsgAssert(p_entry->mpStructure,("NULL p_entry->mpStructure"));
				// Note: Must not assert if not found, cos could be found later.
                if (p_entry->mpStructure->GetChecksum(nameChecksum,p_checksum)) 
				{
					found=true;
				}	
			}
		}
		
		p_comp=p_comp->mpNext;
	}
	if (assert && !found)
	{
		PrintContents(this);
		Dbg_MsgAssert(0,("Checksum parameter '%s' not found in structure",FindChecksumName(nameChecksum)));
	}	
	
    return found;
}

bool CStruct::GetChecksum(const char *p_paramName, uint32 *p_checksum, EAssertType assert) const
{
	return GetChecksum(Crc::GenerateCRCFromString(p_paramName),p_checksum,assert);
}

// Looks for a name, and if no name component is found, looks for a string instead, and if found
// calculates the checksum of that.
bool CStruct::GetChecksumOrStringChecksum(uint32 nameChecksum, uint32 *p_checksum, EAssertType assert) const
{
	Dbg_MsgAssert(p_checksum,("NULL p_checksum"));
	
	if (GetChecksum(nameChecksum,p_checksum,assert))
	{
		return true;
	}
	
	const char *p_string=NULL;
	if (GetString(nameChecksum,&p_string,assert))
	{
		Dbg_MsgAssert(p_string,("NULL p_string ?"));
		*p_checksum=Crc::GenerateCRCFromString(p_string);
		return true;
	}
	
	return false;	
}

bool CStruct::GetChecksumOrStringChecksum(const char *p_paramName, uint32 *p_checksum, EAssertType assert) const
{
	return GetChecksumOrStringChecksum(Crc::GenerateCRCFromString(p_paramName),p_checksum,assert);
}

// Infinite recursion of ContainsComponentNamed could occur, for example if we have a global structure foo={foo}
// which is trying to include itself.
// This counter will make an assert go off if ContainsComponentNamed tries to recurse a suspicious number of times.
#ifdef __NOPT_ASSERT__ 
static uint32 s_num_contains_component_named_recursions=0;
#endif

// Checks to see if there is any component with the given name.
// This is similar to ContainsFlag, except it doesn't care what the type is.
// This function is used by the script function GotParam.
bool CStruct::ContainsComponentNamed(uint32 checksum) const
{
	Dbg_MsgAssert(s_num_contains_component_named_recursions<10,("Possible infinite recursion of ContainsComponentNamed when searching for component named '%s'",FindChecksumName(checksum)));
	#ifdef __NOPT_ASSERT__
	++s_num_contains_component_named_recursions;
	#endif
	
    CComponent *p_comp=mp_components;
    while (p_comp)
    {
		if (p_comp->mNameChecksum==checksum)
		{
			// The name of the component matches, so return true.
			Dbg_MsgAssert(s_num_contains_component_named_recursions,("Eh ?"));
			#ifdef __NOPT_ASSERT__
			--s_num_contains_component_named_recursions;
			#endif
			return true;
		}
			
		if (p_comp->mNameChecksum==0)
		{
			// It's an unnamed component, so maybe it's a single isolated name, or maybe it
			// is a name referring to a structure defined elsewhere ...
			if (p_comp->mType==ESYMBOLTYPE_NAME)
			{
				// It is a name. Check first to see if it is the name being searched for ...
				uint32 ch=p_comp->mChecksum;
				if (ch==checksum)
				{
					// It is.
					Dbg_MsgAssert(s_num_contains_component_named_recursions,("Eh ?"));
					#ifdef __NOPT_ASSERT__
					--s_num_contains_component_named_recursions;
					#endif
					return true;
				}
				
				// Oh well, maybe the name is referring to a global structure ...
				CSymbolTableEntry *p_global=Resolve(ch);
				if (p_global)
				{
					// It is a global ...
					if (p_global->mType==ESYMBOLTYPE_STRUCTURE)
					{
						// It is a structure, so call this function on that.
						Dbg_MsgAssert(p_global->mpStructure,("NULL p_global->mpStructure"));
						if (p_global->mpStructure->ContainsComponentNamed(checksum))
						{
							Dbg_MsgAssert(s_num_contains_component_named_recursions,("Eh ?"));
							#ifdef __NOPT_ASSERT__
							--s_num_contains_component_named_recursions;
							#endif
							return true;
						}	
					}	
				}
			}	
		}	
		
        p_comp=p_comp->mpNext;
    }

	Dbg_MsgAssert(s_num_contains_component_named_recursions,("Eh ?"));
	#ifdef __NOPT_ASSERT__
	--s_num_contains_component_named_recursions;
	#endif
	return false;
}

bool CStruct::ContainsComponentNamed(const char *p_name) const
{
	return ContainsComponentNamed(Crc::GenerateCRCFromString(p_name));
}

// Infinite recursion of ContainsFlag could occur, for example if we have a global structure foo={foo}
// which is trying to include itself.
// This counter will make an assert go off if ContainsFlag tries to recurse a suspicious number of times.
#ifdef __NOPT_ASSERT__ 
static uint32 s_num_contains_flag_recursions=0;
#endif

// Returns true if the structure contains an unnamed component of type Name, whose checksum
// matches that passed.
// Eg, the structure Foo={Type=type_car Position=(0,0,0) CreatedAtStart} does contain an
// unnamed component of type name, with value GenerateCRC("CreatedAtStart"), so CreatedAtStart
// is like a flag.
bool CStruct::ContainsFlag(uint32 checksum) const
{
	Dbg_MsgAssert(s_num_contains_flag_recursions<10,("Possible infinite recursion of ContainsFlag when searching for flag '%s'",FindChecksumName(checksum)));
	#ifdef __NOPT_ASSERT__
	++s_num_contains_flag_recursions;
	#endif
	
    CComponent *p_comp=mp_components;
    while (p_comp)
    {
        if (p_comp->mNameChecksum==0 && p_comp->mType==ESYMBOLTYPE_NAME)
		{
			// Found an unnamed component of type name.
			uint32 name_checksum=p_comp->mChecksum;
			
			if (name_checksum==checksum)
			{	
				Dbg_MsgAssert(s_num_contains_flag_recursions,("Eh ?"));
				#ifdef __NOPT_ASSERT__
				--s_num_contains_flag_recursions;
				#endif
				return true;
			}
			
			// The component might be a structure.
			CSymbolTableEntry *p_global=Resolve(name_checksum);
			if (p_global)
			{
				// It is a global ...
				if (p_global->mType==ESYMBOLTYPE_STRUCTURE)
				{
					// It is a structure so check that structure to see if it contains the flag.
					Dbg_MsgAssert(p_global->mpStructure,("NULL p_global->mpStructure"));
					if (p_global->mpStructure->ContainsFlag(checksum))
					{
						Dbg_MsgAssert(s_num_contains_flag_recursions,("Eh ?"));
						#ifdef __NOPT_ASSERT__
						--s_num_contains_flag_recursions;
						#endif
						return true;
					}	
				}	
			}
		}	
		
        p_comp=p_comp->mpNext;
    }
	
	Dbg_MsgAssert(s_num_contains_flag_recursions,("Eh ?"));
	#ifdef __NOPT_ASSERT__
	--s_num_contains_flag_recursions;
	#endif
    return false;
}

bool CStruct::ContainsFlag(const char *p_name) const
{
	return ContainsFlag(Crc::GenerateCRCFromString(p_name));
}

// Returns true if this structure contains p_struct as a substructure anywhere.
// This is used in CScript::set_script, to determine whether clearing a structure
// will result in the clearing of another structure.
bool CStruct::References(CStruct *p_struct)
{
	if (p_struct==this)
	{
		return true;
	}
	
    CComponent *p_comp=mp_components;
    while (p_comp)
    {
        if (p_comp->mType==ESYMBOLTYPE_STRUCTURE)
		{
			Dbg_MsgAssert(p_comp->mpStructure,("NULL p_comp->mpStructure"));
			if (p_comp->mpStructure->References(p_struct))
			{
				return true;
			}
		}		
        p_comp=p_comp->mpNext;
    }
	
	return false;
}


///////////////////////////////////////////////////////////////////////////////////
// TODO: Remove the following Get... functions at some point.
// They are only included to provide back compatibility with the old code without
// having to change thousands of occurrences.
//
bool CStruct::GetText(uint32 nameChecksum, const char **pp_text, bool assert) const
{
	return GetString(nameChecksum,pp_text,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetText(const char *p_paramName, const char **pp_text, bool assert) const
{
	return GetString(p_paramName,pp_text,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetLocalText(uint32 nameChecksum, const char **pp_text, bool assert) const
{
	return GetString(nameChecksum,pp_text,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetLocalText(const char *p_paramName, const char **pp_text, bool assert) const
{
	return GetString(p_paramName,pp_text,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetInteger(uint32 nameChecksum, int *p_integerValue, bool assert) const
{
	return GetInteger(nameChecksum,p_integerValue,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetInteger(const char *p_paramName, int *p_integerValue, bool assert) const
{
	return GetInteger(p_paramName,p_integerValue,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetFloat(uint32 nameChecksum, float *p_float, bool assert) const
{
	return GetFloat(nameChecksum,p_float,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetFloat(const char *p_paramName, float *p_floatValue, bool assert) const
{
	return GetFloat(p_paramName,p_floatValue,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetVector(uint32 nameChecksum, Mth::Vector *p_vector, bool assert) const
{
	return GetVector(nameChecksum,p_vector,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetVector(const char *p_paramName, Mth::Vector *p_vector, bool assert) const
{
	return GetVector(p_paramName,p_vector,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetPair(uint32 nameChecksum, CPair *p_pair, bool assert) const
{
	float x=0.0f;
	float y=0.0f;
	bool ret_val=GetPair(nameChecksum,&x,&y,assert?ASSERT:NO_ASSERT);
	Dbg_MsgAssert(p_pair,("NULL p_pair"));
	p_pair->mX=x;
	p_pair->mY=y;
	return ret_val;
}

bool CStruct::GetPair(const char *p_paramName, CPair *p_pair, bool assert) const
{
	float x=0.0f;
	float y=0.0f;
	bool ret_val=GetPair(p_paramName,&x,&y,assert?ASSERT:NO_ASSERT);
	Dbg_MsgAssert(p_pair,("NULL p_pair"));
	p_pair->mX=x;
	p_pair->mY=y;
	return ret_val;
}

bool CStruct::GetStructure(uint32 nameChecksum, CStruct **pp_struct, bool assert) const
{
	return GetStructure(nameChecksum,pp_struct,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetStructure(const char *p_paramName, CStruct **pp_struct, bool assert) const
{
	return GetStructure(p_paramName,pp_struct,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetChecksum(uint32 nameChecksum, uint32 *p_checksum, bool assert) const
{
	return GetChecksum(nameChecksum,p_checksum,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetChecksum(const char *p_paramName, uint32 *p_checksum, bool assert) const
{
	return GetChecksum(p_paramName,p_checksum,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetChecksumOrStringChecksum(uint32 nameChecksum, uint32 *p_checksum, bool assert) const
{
	return GetChecksumOrStringChecksum(nameChecksum,p_checksum,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetChecksumOrStringChecksum(const char *p_paramName, uint32 *p_checksum, bool assert) const
{
	return GetChecksumOrStringChecksum(p_paramName,p_checksum,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetArray(uint32 nameChecksum, CArray **pp_array, bool assert) const
{
	return GetArray(nameChecksum,pp_array,assert?ASSERT:NO_ASSERT);
}

bool CStruct::GetArray(const char *p_paramName, CArray **pp_array, bool assert) const
{
	return GetArray(p_paramName,pp_array,assert?ASSERT:NO_ASSERT);
}

#if 0
// Go throught the raw pool and see which entries are valid, and dump them
//	uint8 *				mp_buffer;
//	uint8 *				mp_buffer_end;
//	int					m_totalItems; // that we have room for
//	int					m_itemSize;


void DumpStructs()
{
	Mem::CCompactPool * p_pool = Mem::CPoolable<CStruct>::sp_pool[Mem::CPoolable<CStruct>::s_currentPool];
	CStruct *p_struct = (CStruct *)p_pool->mp_buffer;
	while (p_struct <  (CStruct *)p_pool->mp_buffer_end)
	{
		if (p_pool->IsInPool(p_struct))
		{
			PrintContents(p_struct);
		}
		
		p_struct++;		
	}
}

#endif


///////////////////////////////////////////////////////////////////////////////////
		
} // namespace Script

