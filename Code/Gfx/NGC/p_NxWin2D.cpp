///////////////////////////////////////////////////////////////////////////////
// p_NxWin2D.cpp

#include 	"Gfx/Ngc/p_NxWin2D.h"

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

CNgcWindow2D::CNgcWindow2D(int x, int y, int width, int height) : CWindow2D(x, y, width, height)
{
//	mp_plat_window = new NxNgc::SScissorWindow();

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CNgcWindow2D::~CNgcWindow2D()
{
//	if (mp_plat_window)
//	{
//		delete mp_plat_window;
//	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CNgcWindow2D::plat_update_engine()
{
	Dbg_Assert(m_xpos >= 0);
	Dbg_Assert(m_ypos >= 0);

	int x1 = m_xpos + m_width - 1;
	int y1 = m_ypos + m_height - 1;

	Dbg_Assert(x1 >= 0);
	Dbg_Assert(y1 >= 0);

//	mp_plat_window->SetScissor(m_xpos, m_ypos, x1, y1);
	m_left		= m_xpos;
	m_top		= m_ypos;
	m_right 	= x1;
	m_bottom	= y1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CWindow2DManager::s_plat_alloc_window2d_pool()
{
	for (int i = 0; i < vMAX_WINDOW_INSTANCES; i++)
	{
	   	CNgcWindow2D *p_window = new CNgcWindow2D;
		p_window->mp_next = sp_window_list;
		sp_window_list = p_window;
	}
}

} // Namespace Nx  			
				
				

