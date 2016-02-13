///////////////////////////////////////////////////////////////////////////////////////
//
// vecpair.cpp		KSH 17 Oct 2001
//
// Just defines the constructors and pool statics.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/vecpair.h>

DefinePoolableClass(Script::CVector);
DefinePoolableClass(Script::CPair);

namespace Script
{

CVector::CVector()
{
	// Initialise everything. CVector is not derived from CClass so we don't get
	// the auro-zeroing.
	mX=mY=mZ=0.0f;
}

// Note: No destructor needed.


CPair::CPair()
{
	// Initialise everything. CPair is not derived from CClass so we don't get
	// the auro-zeroing.
	mX=mY=0.0f;
}

// Note: No destructor needed.

} // namespace Script

