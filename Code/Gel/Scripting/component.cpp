///////////////////////////////////////////////////////////////////////////////////////
//
// component.cpp		KSH 17 Oct 2001
//
// Notes:
// When a CComponent is deleted, it does not delete any CStruct, CArray or string that
// it may contain a pointer to. This is so that a cyclic dependency between component and struct is avoided.
// It is up to the parent CStruct to clean these up before deleting the CComponent.
// 
// This cpp just declares the constructor, and the pool statics.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/component.h>

#ifndef	__SCRIPTING_SCRIPTDEFS_H
#include <gel/scripting/scriptdefs.h>
#endif

DefinePoolableClass(Script::CComponent);

namespace Script
{

// Used in parkgen.cpp, but only in a debug message, so make it just return 0 for now.
uint32 GetNumUsedComponents()
{
	return 0;
}

CComponent::CComponent()
{
	// Initialise everything. CComponent is not derived from CClass so we don't get
	// the auro-zeroing.
	mType=ESYMBOLTYPE_NONE;
	mNameChecksum=NO_NAME;
	mScriptSize=0;
	mUnion=0;
	mpNext=NULL;
}

// The destructor does not delete anything to avoid cyclic dependencies, so just assert
// if it look like there could be a non-NULL pointer left.
#ifdef __NOPT_ASSERT__
CComponent::~CComponent()
{
	Dbg_MsgAssert(mUnion==0,("CComponent still contains data, possibly an undeleted pointer"));
	Dbg_MsgAssert(mScriptSize==0,("CComponent::mScriptSize not zero in destructor ?"));
}
#endif

// Note: No destructor.
// When a CComponent is deleted, it does not delete any CStruct, CArray or string that
// it may contain a pointer to.
// This is so that a cyclic dependency between component and struct is avoided, because to
// delete a CStruct from here would require the inclusion of struct.h.

// It is up to the parent CStruct to delete anything in the component before deleting the CComponent.

} // namespace Script

