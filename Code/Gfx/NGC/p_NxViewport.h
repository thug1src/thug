///////////////////////////////////////////////////////////////////////////////
// p_NxViewport.h

#ifndef	__GFX_P_NX_VIEWPORT_H__
#define	__GFX_P_NX_VIEWPORT_H__

#include 	"Gfx/NxViewport.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Machine specific implementation of the CViewport
class	CNgcViewport : public CViewport
{
public:
								CNgcViewport();
								CNgcViewport(const Mth::Rect* rect, Gfx::Camera* cam = NULL);
	virtual						~CNgcViewport();

private:		// It's all private, as it is machine specific
	//virtual void				plat_initialize();

};

} // Namespace Nx  			

#endif

