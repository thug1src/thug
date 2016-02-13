#include <core/defines.h>
#include <gfx/2D/Window.h>
#include <gfx/NxWin2D.h>

#ifdef	__PLAT_NGPS__ 
#include <gfx/NGPS/NX/chars.h>
#endif

namespace Front
{




CWindowElement::CWindowElement()
{
	SetType(CScreenElement::TYPE_WINDOW_ELEMENT);
	mp_clip_window = Nx::CWindow2DManager::sGetWindowInstance(0, 0, 640, 448);
}




CWindowElement::~CWindowElement()
{
	Dbg_Assert(mp_clip_window);
	Nx::CWindow2DManager::sFreeWindowInstance(mp_clip_window);
}


Nx::CWindow2D *	CWindowElement::GetClipWindow() const
{
	return mp_clip_window;
}


#if 0
void CWindowElement::drawMainPart()
{
#ifdef	__PLAT_NGPS__ 
	NxPs2::SetTextWindow(0,639,0,447);						// a full-screen clipping window
#else
	printf ("WARNING: drawMainPart not the same on this platform....\n");
#endif
}
#endif




}
