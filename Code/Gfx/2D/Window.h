#ifndef __GFX_2D_WINDOW_H__
#define __GFX_2D_WINDOW_H__

#include <gfx/2D/ScreenElement2.h>

namespace Nx
{
	class CWindow2D;
}

namespace Front
{

class CWindowElement : public Front::CScreenElement
{
public:

							CWindowElement();
	virtual					~CWindowElement();

	void					SetClipWindow(Nx::CWindow2D *p_window);		// This should probably be protected or private
	Nx::CWindow2D *			GetClipWindow() const;

protected:
	Nx::CWindow2D *			mp_clip_window;

};

}

#endif
