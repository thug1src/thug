#include <core/defines.h>
#include <eekernel.h>
#include "resource.h"

namespace NxPs2
{


bool			CSystemResources::s_resource_used[vNUM_RESOURCES];

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 			CSystemResources::sInitResources()
{
	for (int i = 0; i < vNUM_RESOURCES; i++)
	{
		s_resource_used[i] = false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool 			CSystemResources::sRequestResource(EResourceType resource)
{
	Dbg_Assert(resource < vNUM_RESOURCES);
	
	bool old_result;

	// We need to do this with interrupts off, just in case
	DI();
	old_result = s_resource_used[resource];
	s_resource_used[resource] = true;			// We know it is reserved now
	EI();

	return !old_result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 			CSystemResources::sFreeResource(EResourceType resource)
{
	Dbg_Assert(resource < vNUM_RESOURCES);
	Dbg_Assert(s_resource_used[resource]);

	// We need to do this with interrupts off, just in case
	DI();
	s_resource_used[resource] = false;
	EI();
}

} // namespace NxPs2

