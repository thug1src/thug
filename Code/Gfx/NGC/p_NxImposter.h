///////////////////////////////////////////////////////////////////////////////
// p_NxFont.h

#ifndef	__GFX_P_NX_IMPOSTER_H__
#define	__GFX_P_NX_IMPOSTER_H__

#include 	"gfx/NxImposter.h"

namespace Nx
{
	
/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CImposterGroup
class CNgcImposterGroup : public CImposterGroup
{
	public:
								CNgcImposterGroup();
	virtual						~CNgcImposterGroup();


	private:					// It's all private, as it is machine specific

	// Machine specific members
};

} // Namespace Nx  			

#endif
