///////////////////////////////////////////////////////////////////////////////
// p_NxWin2D.cpp

#include 	"Gfx/NGPS/p_NxWin2D.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

/////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of the CText

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Window2D::CPs2Window2D(int x, int y, int width, int height) : CWindow2D(x, y, width, height)
{
	mp_plat_window = new NxPs2::SScissorWindow();

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Window2D::~CPs2Window2D()
{
	if (mp_plat_window)
	{
		delete mp_plat_window;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Window2D::plat_update_engine()
{
	int x0 = (int) (((float) m_xpos) * NxPs2::SDraw2D::GetScreenScaleX());
	int y0 = (int) (((float) m_ypos) * NxPs2::SDraw2D::GetScreenScaleY());

	Dbg_Assert(x0 >= 0);
	Dbg_Assert(y0 >= 0);

	int x1 = x0 + (int) (((float) m_width ) * NxPs2::SDraw2D::GetScreenScaleX()) - 1;
	int y1 = y0 + (int) (((float) m_height) * NxPs2::SDraw2D::GetScreenScaleY()) - 1;

	Dbg_Assert(x1 >= 0);
	Dbg_Assert(y1 >= 0);

	mp_plat_window->SetScissor(x0, y0, x1, y1);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CWindow2DManager::s_plat_alloc_window2d_pool()
{
	for (int i = 0; i < vMAX_WINDOW_INSTANCES; i++)
	{
	   	CPs2Window2D *p_window = new CPs2Window2D;
		p_window->mp_next = sp_window_list;
		sp_window_list = p_window;
	}
}

} // Namespace Nx  			
				
				
