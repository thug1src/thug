///////////////////////////////////////////////////////////////////////////////
// p_NxWin2D.h

#ifndef	__GFX_P_NX_WIN2D_H__
#define	__GFX_P_NX_WIN2D_H__

#include 	"Gfx/NxWin2D.h"
#include 	"Gfx/Xbox/NX/sprite.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

/////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of the CWindow2D
class	CXboxWindow2D : public CWindow2D
{
public:
								CXboxWindow2D( int x = 0, int y = 0, int width = 640, int height = 480 );
								CXboxWindow2D( const Mth::Rect & win_rect);
	virtual						~CXboxWindow2D();

private:
	//
	virtual void				plat_update_engine();		// Update engine primitives
};

} // Namespace Nx  			

#endif
