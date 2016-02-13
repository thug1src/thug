#ifndef __RESOURCE_H
#define __RESOURCE_H

#include	<core/defines.h>
#include	<core/math.h>

namespace NxPs2
{

// This static class locks certain system resources (like scratchpad and VU0 memory)
// for code to use exclusively.  Right now, there is no mechanism to lock out just
// parts of memory, but it may be added in the future if needed.
class CSystemResources
{

public:
	// Resource types
	enum EResourceType {
		vVU0_MEMORY = 0,
		vSCRATCHPAD_MEMORY,
		vNUM_RESOURCES
	};

	//-------------------------------------------

	static void sInitResources();								// Call only at the beginning!

	static bool sRequestResource(EResourceType resource); 		// Returns true if it was able to lock it
	static void sFreeResource(EResourceType resource);

private:
	static bool s_resource_used[vNUM_RESOURCES];

};


} // namespace NxPs2



#endif // __RESOURCE_H

