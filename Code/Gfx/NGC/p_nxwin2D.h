///////////////////////////////////////////////////////////////////////////////
// p_NxWin2D.h

#ifndef	__GFX_P_NX_WIN2D_H__
#define	__GFX_P_NX_WIN2D_H__

#include 	"Gfx/NxWin2D.h"
#include 	"Gfx/Ngc/NX/sprite.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

/////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of the CWindow2D
class	CNgcWindow2D : public CWindow2D
{
public:
								CNgcWindow2D(int x = 0, int y = 0, int width = 640, int height = 448);
								CNgcWindow2D(const Mth::Rect & win_rect);
	virtual						~CNgcWindow2D();

//	NxNgc::SScissorWindow *		GetEngineWindow() const;

private:
	//
	virtual void				plat_update_engine();		// Update engine primitives

	// Machine specific members
//	NxNgc::SScissorWindow *		mp_plat_window;				// Pointer to engine window
	short						m_left, m_right, m_top, m_bottom;
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//inline NxNgc::SScissorWindow *	CNgcWindow2D::GetEngineWindow() const
//{
//	return mp_plat_window;
//}

} // Namespace Nx  			

#endif

