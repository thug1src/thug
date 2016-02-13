#ifndef	__SCRIPTING_COMPONENT_H
#define	__SCRIPTING_COMPONENT_H

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

class CPair;
class CVector;
class CStruct;
class CArray;

// Note: This is not derived from CClass to avoid the extra memory overhead due to the virtual destructor.
// There will be loads of CComponents, in THPS3 there were 58000.
#ifdef __PLAT_WN32__
class CComponent
#else
class CComponent : public Mem::CPoolable<CComponent>
#endif
{
public:	
	CComponent();
	#ifdef __NOPT_ASSERT__
	~CComponent();
	#endif

	// These cannot be defined because it would cause a cyclic dependency, because
	// a CComponent member function can't create things. So declare them but leave them undefined
	// so that it will not link if they are attempted to be used.
	CComponent( const CComponent& rhs );
	CComponent& operator=( const CComponent& rhs );

	// These are used when interpreting switch statements and expressions
	bool operator==( const CComponent& v ) const;
	bool operator!=( const CComponent& v ) const;
	bool operator<( const CComponent& v ) const;
	bool operator>( const CComponent& v ) const;
	bool operator<=( const CComponent& v ) const;
	bool operator>=( const CComponent& v ) const;

	// Making this use just one byte keeps the size of CComponent to 16 bytes.
	// There are 64000 CComponents so they need to be as small as possible.
	// It seems that the compact pool class has an overhead of one byte, so if the
	// size of this class is 3 mod 4 it will be making optimal use of space.
	// Hence we have 2 bytes spare if needed later.
	uint8 mType;
	
	// Well, this has used up those 2 spare bytes mentioned above.
	// This is the size of the mpScript buffer in the case of mType being ESYMBOLTYPE_QSCRIPT
	// The size is needed when copying a component, so that the new component knows how big a
	// buffer to allocate. In theory the size could be calculated, but that would introduce
	// a circular dependency because struct.cpp would have to include parse.h in order to call
	// SkipOverScript.
	uint16 mScriptSize;
	
	uint32 mNameChecksum;	
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
		uint32 mUnion; // For when all the above need to be zeroed 
	};
	
	CComponent *mpNext;
};
	
void AllocateComponentPool(uint32 maxComponents);
void DeallocateComponentPool();
	
uint32 GetNumUsedComponents();
	
} // namespace Script

#endif // #ifndef	__SCRIPTING_COMPONENT_H
