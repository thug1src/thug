///////////////////////////////////////////////////////////////////////////////
// NxWin2D.h



#ifndef	__GFX_NXWIN2D_H__
#define	__GFX_NXWIN2D_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __CORE_MATH_RECT_H
#include <core/math/rect.h>
#endif

namespace Nx
{

// Forward declarations
class CWindow2DManager;

//////////////////////////////////////////////////////////////////////////////
// Holds a string of text that needs to be displayed this frame
class CWindow2D
{
public:
							CWindow2D(int x = 0, int y = 0, int width = 640, int height = 448);
							CWindow2D(const Mth::Rect & win_rect);
	virtual					~CWindow2D();

	// Position of screen.
	int						GetXPos() const;
	int						GetYPos() const;
	void					SetPos(int x, int y);

	// Size of screen.
	int						GetWidth() const;
	int						GetHeight() const;
	void					SetSize(int width, int height);

protected:
	int						m_xpos;
	int						m_ypos;
	int						m_width;
	int						m_height;

	// For free list in CWindow2DManager
	CWindow2D *				mp_next;

private:
	//
	virtual void			plat_update_engine();

	friend CWindow2DManager;

};

//////////////////////////////////////////////////////////////////////////////
// Static class for memory management of CWindow2Ds
class CWindow2DManager
{
public:
	//
	static void				sAllocWindow2DPool();
	static CWindow2D *		sGetWindowInstance(int x = 0, int y = 0, int width = 640, int height = 448);
	static void				sFreeWindowInstance(CWindow2D *p_window);

private:
	// Constants
	enum {
		vMAX_WINDOW_INSTANCES = 10
	};

	// Because it is static, it is declared here, but defined in p_NxWin2D.cpp
	static void				s_plat_alloc_window2d_pool();

	// Array of Text requests
	static CWindow2D *	   	sp_window_list;
};

/////////////////////////////////////////////////////////
// CText inline function
inline int					CWindow2D::GetXPos() const
{
	return m_xpos;
}

inline int					CWindow2D::GetYPos() const
{
	return m_ypos;
}

inline int					CWindow2D::GetWidth() const
{
	return m_width;
}

inline int					CWindow2D::GetHeight() const
{
	return m_height;
}

}

#endif // 

