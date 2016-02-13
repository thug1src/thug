#ifndef __GFX_2D_SCROLLINGMENU_H__
#define __GFX_2D_SCROLLINGMENU_H__

#include <gfx/2D/ScreenElement2.h>

namespace Front
{

class CBaseMenu;
	
class CBaseScrollingMenu : public Front::CScreenElement, public Obj::CEventListener
{
public:

							CBaseScrollingMenu();
	virtual					~CBaseScrollingMenu();

	void					SetInternalJust(float h, float v) {m_internal_just_x = h; m_internal_just_y = v;}	
	
	virtual void			SetProperties(Script::CStruct *pProps);
	virtual bool			PassTargetedEvent(Obj::CEvent *pEvent, bool broadcast = false);

protected:

	void					pass_event_to_listener(Obj::CEvent *pEvent);
	virtual void 			update();

	void					get_the_menu();
	void					put_element_in_view(const CScreenElementPtr &pElement);
    	
	void					compute_element_area(const CScreenElementPtr &pElement, float &elementX, float &elementY, float &elementW, float &elementH);
	
	CBaseMenu *				mp_the_menu;
	uint32					m_selected_element_id;
	
	float					m_internal_just_x, m_internal_just_y;

	bool					m_in_focus;
	bool					m_needs_update;

	float					m_top_or_left_line;
	float					m_scroll_speed;

	// set to negative if we aren't limiting
	int						m_num_items_to_display;
	float					m_window_dimension;
	// set to negative if not used
	float					m_regular_space_amount;

	bool					m_is_vertical;
};




class CVScrollingMenu : public CBaseScrollingMenu
{
public:
							CVScrollingMenu();
	virtual					~CVScrollingMenu();
};




class CHScrollingMenu : public CBaseScrollingMenu
{
public:
							CHScrollingMenu();
	virtual					~CHScrollingMenu();
};




};

#endif
