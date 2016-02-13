///////////////////////////////////////////////////////////////////////////////
// NxWin2D.cpp

#include "gfx/NxWin2D.h"

namespace	Nx
{
// These functions are the platform independent part of the interface to 
// the platform specific code
// parameter checking can go here....
// although we might just want to have these functions inline, or not have them at all?
					 
///////////////////////////////////////////////////////////////////////////////
// CWindow2D

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CWindow2D::CWindow2D(int x, int y, int width, int height) : 
	m_xpos(x),
	m_ypos(y),
	m_width(width),
	m_height(height),
	mp_next(NULL)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CWindow2D::~CWindow2D() 
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CWindow2D::SetPos(int x, int y)
{
	m_xpos = x;
	m_ypos = y;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CWindow2D::SetSize(int width, int height)
{
	m_width = width;
	m_height = height;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CWindow2D::plat_update_engine()
{
	printf ("STUB: PlatUpdateEngine\n");
}

///////////////////////////////////////////////////////////////////////////////
// CWindow2DManager

CWindow2D *			CWindow2DManager::sp_window_list = NULL;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CWindow2DManager::sAllocWindow2DPool()
{
	s_plat_alloc_window2d_pool();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CWindow2D *		CWindow2DManager::sGetWindowInstance(int x, int y, int width, int height)
{
	CWindow2D *p_window = sp_window_list;

	if (p_window)
	{
		sp_window_list = p_window->mp_next;
	} else {
		Dbg_MsgAssert(0, ("Out of CWindow2D Instances"));
	}

	// Initialize
	p_window->SetPos(x, y);
	p_window->SetSize(width, height);

	return p_window;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CWindow2DManager::sFreeWindowInstance(CWindow2D *p_window)
{
	p_window->mp_next = sp_window_list;
	sp_window_list = p_window;
}

}


