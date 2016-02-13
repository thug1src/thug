#ifndef	__SCRIPTING_VECPAIR_H
#define	__SCRIPTING_VECPAIR_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

namespace Script
{

// Note: These are not derived from CClass to avoid the extra memory overhead due to the virtual destructor.

#ifdef __PLAT_WN32__
class CVector
#else
class CVector : public Mem::CPoolable<CVector>
#endif
{
public:
	CVector();
	// No copy constructor or assignement operators needed, the defaults will work.

	union
	{
		CVector *mpNext;
		float mX;
	};	
	
	float mY;
	float mZ;
};

#ifdef __PLAT_WN32__
class CPair
#else
class CPair : public Mem::CPoolable<CPair>
#endif
{
public:
	CPair();
	// No copy constructor or assignement operators needed, the defaults will work.

	union
	{
		CPair *mpNext;
		float mX;
	};	
	
	float mY;
};

} // namespace Script

#endif // #ifndef	__SCRIPTING_VECPAIR_H
