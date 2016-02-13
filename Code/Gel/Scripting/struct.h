#ifndef	__SCRIPTING_STRUCT_H
#define	__SCRIPTING_STRUCT_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

#ifndef	__SCRIPTING_SCRIPTDEFS_H
#include <gel/scripting/scriptdefs.h> // For EAssertType
#endif

#ifndef	__SCRIPTING_SYMBOLTYPE_H
#include <gel/scripting/symboltype.h> // For ESymbolType
#endif

namespace Mth
{
class Vector;
}

namespace Script
{

class CPair; // TODO: Remove once the old GetPair member function is removed.

class CComponent;
class CScript;
class CArray;
struct SWhatever;

// This defines a reference to a script that is defined in a CStruct.
// A pointer to one of these can then be passed to CScript::SetScript.
// Only exists as a convenient way of passing the data from GetScript to SetScript.
struct SStructScript
{
	// The name of the script, which is the same as its parameter name in the source structure.
	uint32 mNameChecksum;
	// Pointer to the script data in the structure.
	// Note that this means that this pointer will become invalid once the source structure is deleted.
	// OK though, because SStructScript is only used to pass the info to CScript::SetScript, which
	// will make its own copy of the script data.
	uint8 *mpScriptTokens;
	
	SStructScript() {mNameChecksum=NO_NAME; mpScriptTokens=NULL;}
};

#ifdef __PLAT_WN32__
class CStruct
#else
class CStruct : public Mem::CPoolable<CStruct>
#endif
{
	// Head pointer of the list of components.
	CComponent *mp_components;
    
	#ifdef __NOPT_ASSERT__ 
	// The script that created this structure. Only valid (non NULL) if this is 
	// a mpFunctionParams structure created by a script.
	// Needed so that if the Get... functions assert they can print info about 
	// the parent script, eg the line number of the error.
	CScript *mp_parent_script;
	#endif

	void init();
	bool search_for(uint32 nameChecksum, ESymbolType type, SWhatever *p_value) const;
	
public:
    CStruct();
    ~CStruct();

	// These are defined, and will create new instances of any entities pointed to by the
	// source structure, so the two structures will not share any pointers.
	// Careful when using these though, because if copying a complex structure with lots
	// of nested sub-structures and arrays it could potentially take a long time to execute.
	CStruct( const CStruct& rhs );
	CStruct& operator=( const CStruct& rhs );
	
	// This will merge the contents of the rhs into this structure.
	// Functionally the same as the old AppendStructure function.
	CStruct& operator+=( const CStruct& rhs );
	
    // Deletes all components, deleting any arrays or structures referenced by the components.
    void Clear();

	// Removes the component with the given name.
	void RemoveComponent(uint32 nameChecksum);
	void RemoveComponent(const char *p_name);

	// Same as RemoveComponent except the type must match too.
	void RemoveComponentWithType(uint32 nameChecksum, uint8 type);
	void RemoveComponentWithType(const char *p_name, uint8 type);
	
	// Removes the flag (unnamed name) with the given name.
	void RemoveFlag(uint32 checksum);
	void RemoveFlag(const char *p_name);

	// Adds p_comp to the end of the list of components, removing any existing components with
	// the same name.
	void AddComponent(CComponent *p_comp);
	
	#ifdef __NOPT_ASSERT__ 
	void SetParentScript(CScript *p_parentScript);
	#endif

	// If a component of the same name and type already exists it will be replaced with the new value.
	// If a component of the same name but different type exists, it will assert.
	// NameChecksum can be 0, meaning no name.
	// If all OK, returns a pointer to the new component.
    
	// These will copy in the actual string data, not store the pointer.
	void AddString(uint32 nameChecksum, const char *p_string);
	void AddString(const char *p_name, const char *p_string);
	void AddLocalString(uint32 nameChecksum, const char *p_string);
	void AddLocalString(const char *p_name, const char *p_string);
	
	void AddInteger(uint32 nameChecksum, int integer);
	void AddInteger(const char *p_name, int integer);
	
	void AddFloat(uint32 nameChecksum, float float_val);
	void AddFloat(const char *p_name, float float_val);

	void AddChecksum(uint32 nameChecksum, uint32 checksum);
	void AddChecksum(const char *p_name, uint32 checksum);
	
	void AddVector(uint32 nameChecksum, float x, float y, float z);
	void AddVector(const char* p_name, float x, float y, float z);
	void AddVector( uint32 nameChecksum, Mth::Vector vector );
	void AddVector( const char* p_name, Mth::Vector vector );

	
	void AddPair(uint32 nameChecksum, float x, float y);
	void AddPair(const char *p_name, float x, float y);

	// Creates a new array & copies in the contents of the passed array.
	void AddArray(uint32 nameChecksum, const CArray *p_array);
	void AddArray(const char *p_name, const CArray *p_array);

	// Stores the passed pointer. Faster than copying the contents.
	// The passed pointer is no longer const because the CStruct will delete it when destroyed.
	void AddArrayPointer(uint32 nameChecksum, CArray *p_array);
	void AddArrayPointer(const char *p_name, CArray *p_array);

	// Creates a new structure & copies in the contents of the passed structure.
	void AddStructure(uint32 nameChecksum, const CStruct *p_structure);
	void AddStructure(const char *p_name, const CStruct *p_structure);
	
	// Stores the passed pointer. Faster than copying the contents.
	// The passed pointer is no longer const because the CStruct will delete it when destroyed.
	void AddStructurePointer(uint32 nameChecksum, CStruct *p_structure);
	void AddStructurePointer(const char *p_name, CStruct *p_structure);

	// Creates a new script component and copies in the passed script.
	void AddScript(uint32 nameChecksum, const uint8 *p_scriptTokens, uint32 size);
	void AddScript(const char *p_name, const uint8 *p_scriptTokens, uint32 size);
	
	///////////////////////////////////////////////////////////////////////////////////
	// TODO: Remove all these AddComponent functions at some point.
	// They are only included to provide back compatibility with the old code without
	// having to change thousands of occurrences of calls to AddComponent.
	// Gradually phase out the old AddComponent instead.
	//
	
    // String or local string.
	void AddComponent(uint32 nameChecksum, ESymbolType type, const char *p_string); // This will copy the actual string data
    // Integer or any other 4 byte value.
	void AddComponent(uint32 nameChecksum, ESymbolType type, int integer);
    // Vector
	void AddComponent(uint32 nameChecksum, float x, float y, float z);
    // Pair
	void AddComponent(uint32 nameChecksum, float x, float y);
	// Array
	void AddComponent(uint32 nameChecksum, CArray *p_array); // Copies the data in the array
	///////////////////////////////////////////////////////////////////////////////////


	bool GetString		(uint32 nameChecksum, 		const char **pp_text, 	EAssertType assert=NO_ASSERT) const;
	bool GetString		(const char *p_paramName, 	const char **pp_text, 	EAssertType assert=NO_ASSERT) const;
    bool GetLocalString	(uint32 nameChecksum, 		const char **pp_text, 	EAssertType assert=NO_ASSERT) const;
	bool GetLocalString	(const char *p_paramName, 	const char **pp_text, 	EAssertType assert=NO_ASSERT) const;
	bool GetInteger		(uint32 nameChecksum, 		int *p_integerValue, 	EAssertType assert=NO_ASSERT) const;
	bool GetInteger		(const char *p_paramName, 	int *p_integerValue, 	EAssertType assert=NO_ASSERT) const;
	bool GetFloat		(uint32 nameChecksum, 		float *p_floatValue,	EAssertType assert=NO_ASSERT) const;
	bool GetFloat		(const char *p_paramName, 	float *p_floatValue, 	EAssertType assert=NO_ASSERT) const;
	bool GetVector		(uint32 nameChecksum, 		Mth::Vector *p_vector, 	EAssertType assert=NO_ASSERT) const;
	bool GetVector		(const char *p_paramName, 	Mth::Vector *p_vector, 	EAssertType assert=NO_ASSERT) const;
	bool GetPair		(uint32 nameChecksum, 		float *p_x, float *p_y,	EAssertType assert=NO_ASSERT) const;
	bool GetPair		(const char *p_paramName, 	float *p_x, float *p_y,	EAssertType assert=NO_ASSERT) const;
	bool GetStructure	(uint32 nameChecksum, 		CStruct **pp_structure,	EAssertType assert=NO_ASSERT) const;
	bool GetStructure	(const char *p_paramName, 	CStruct **pp_structure,	EAssertType assert=NO_ASSERT) const;
	bool GetChecksum	(uint32 nameChecksum, 		uint32 *p_checksum, 	EAssertType assert=NO_ASSERT) const;
	bool GetChecksum	(const char *p_paramName, 	uint32 *p_checksum, 	EAssertType assert=NO_ASSERT) const;
	bool GetArray		(uint32 nameChecksum, 		CArray **pp_array, 		EAssertType assert=NO_ASSERT) const;
	bool GetArray		(const char *p_paramName, 	CArray **pp_array, 		EAssertType assert=NO_ASSERT) const;
	bool GetScript		(uint32 nameChecksum, 		SStructScript *p_structScript, EAssertType assert=NO_ASSERT) const;
	bool GetScript		(const char *p_paramName,	SStructScript *p_structScript, EAssertType assert=NO_ASSERT) const;
	bool GetChecksumOrStringChecksum(uint32 nameChecksum, uint32 *p_checksum, EAssertType assert=NO_ASSERT) const;
	bool GetChecksumOrStringChecksum(const char *p_paramName, uint32 *p_checksum, EAssertType assert=NO_ASSERT) const;
	bool ContainsComponentNamed(uint32 checksum) const;
	bool ContainsComponentNamed(const char *p_name) const;
	bool ContainsFlag	(uint32 checksum) const;
	bool ContainsFlag	(const char *p_name) const;

	///////////////////////////////////////////////////////////////////////////////////
	// TODO: Remove all these GetText functions at some point.
	// They are only included to provide back compatibility with the old code without
	// having to change thousands of occurrences of calls to GetText.
	// Some of them do not have a default value for assert given, to prevent ambiguities with the above functions.
    bool GetText(uint32 nameChecksum, const char **pp_text, bool assert=false) const;
	bool GetText(const char *p_paramName, const char **pp_text, bool assert=false) const;
    bool GetLocalText(uint32 nameChecksum, const char **pp_text, bool assert=false) const;
	bool GetLocalText(const char *p_paramName, const char **pp_text, bool assert=false) const;
	bool GetInteger(uint32 nameChecksum, int *p_integerValue, bool assert) const;
	bool GetInteger(const char *p_paramName, int *p_integerValue, bool assert) const;
	bool GetFloat(uint32 nameChecksum, float *p_float, bool assert) const;
	bool GetFloat(const char *p_paramName, float *p_floatValue, bool assert) const;
	bool GetVector(uint32 nameChecksum, Mth::Vector *p_vector, bool assert) const;
	bool GetVector(const char *p_paramName, Mth::Vector *p_vector, bool assert) const;
	bool GetPair(uint32 nameChecksum, CPair *p_pair, bool assert=false) const;
	bool GetPair(const char *p_paramName, CPair *p_pair, bool assert=false) const;
	bool GetStructure(uint32 nameChecksum, CStruct **pp_struct, bool assert) const;
	bool GetStructure(const char *p_paramName, CStruct **pp_struct, bool assert) const;
	bool GetChecksum(uint32 nameChecksum, uint32 *p_checksum, bool assert) const;
	bool GetChecksum(const char *p_paramName, uint32 *p_checksum, bool assert) const;
	bool GetChecksumOrStringChecksum(uint32 nameChecksum, uint32 *p_checksum, bool assert) const;
	bool GetChecksumOrStringChecksum(const char *p_paramName, uint32 *p_checksum, bool assert) const;
	bool GetArray(uint32 nameChecksum, CArray **pp_array, bool assert) const;
	bool GetArray(const char *p_paramName, CArray **pp_array, bool assert) const;
	///////////////////////////////////////////////////////////////////////////////////
	
	// TODO: Remove at some point. Provided for back-compatibility.
	void AppendStructure(const CStruct *p_struct);

	void AppendComponentPointer(CComponent *p_comp);

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
	// recursionCount is just to catch infinite recursion, so no need to pass it a value.
	void ExpandInto(CStruct *p_dest, int recursionCount=0) const;
	
	// Returns true if the structure contains no components.
	bool IsEmpty() const;
    
	// Searches for a component with the given name, but will also recurse into substructures.
	// Used when resolving the <ParamName> syntax.
	CComponent *FindNamedComponentRecurse(uint32 nameChecksum) const;
	
	// If passed NULL this will return the first (leftmost) component.
	// If passed non-NULL it return the next component (to the right) 
	// Returns NULL if the passed component is the last component.
	CComponent *GetNextComponent(CComponent *p_comp=NULL) const;

	// Returns true if this structure contains p_struct as a substructure anywhere.
	// This is used in CScript::set_script, to determine whether clearing a structure
	// will result in the clearing of another structure.
	bool References(CStruct *p_struct);
	
	#ifdef __NOPT_ASSERT__ 
	CScript *GetParentScript() const;
	bool CheckForDuplicateFlags() const;
	#endif
};

void CleanUpComponent(CComponent *p_comp);
void CopyComponent(CComponent *p_dest, const CComponent *p_source);
void CleanUpArray(CArray *p_array);
void CopyArray(CArray *p_dest, const CArray *p_source);

void DumpStructs();


} // namespace Script

#endif // #ifndef	__SCRIPTING_STRUCT_H
