///////////////////////////////////////////////////////////////////////////////
// p_NxWin2D.h

#ifndef	__GFX_P_NX_WIN2D_H__
#define	__GFX_P_NX_WIN2D_H__

#include 	"Gfx/NxWin2D.h"
#include 	"Gfx/NGPS/NX/sprite.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

/////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of the CWindow2D
class	CPs2Window2D : public CWindow2D
{
public:
								CPs2Window2D(int x = 0, int y = 0, int width = 640, int height = 448);
								CPs2Window2D(const Mth::Rect & win_rect);
	virtual						~CPs2Window2D();

	NxPs2::SScissorWindow *		GetEngineWindow() const;

private:
	//
	virtual void				plat_update_engine();		// Update engine primitives

	// Machine specific members
	NxPs2::SScissorWindow *		mp_plat_window;				// Pointer to engine window
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline NxPs2::SScissorWindow *	CPs2Window2D::GetEngineWindow() const
{
	return mp_plat_window;
}

} // Namespace Nx  			

#endif
