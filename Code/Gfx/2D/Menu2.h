#ifndef __GFX_2D_MENU_H__
#define __GFX_2D_MENU_H__

#include <gfx/2D/ScreenElement2.h>

namespace Front
{

/*
	A base menu is an.
*/
class CBaseMenu : public Front::CScreenElement
{
	friend class CVScrollingMenu;

public:

							CBaseMenu();
	virtual					~CBaseMenu();

	void					SetInternalJust(float h, float v) {m_internal_just_x = h; m_internal_just_y = v;}	
	
	virtual void			SetProperties(Script::CStruct *pProps);
	virtual bool			PassTargetedEvent(Obj::CEvent *pEvent, bool broadcast = false);

	int						GetSelectedIndex() { return m_selected_index; }

	bool					UsingRegularSpacing(float &rRegularSpaceAmount);

protected:

	void					find_focusable_item(int dir, bool include_current, bool updateGridIndex);
	void					change_selection(int dir);
	void					setup_tags();
	void					reposition();

	// both m_selected_index and m_selected_id will have matching, valid settings,
	// or both will be invalid (-1, 0)
	int                     m_selected_index;
    uint32                  m_selected_id;
	bool					m_in_focus;
	int						m_focus_controller; // 0 or 1

	float					m_internal_just_x, m_internal_just_y;
	bool 					m_is_vertical_menu;

	float					m_regular_space_val;
	float					m_padding_scale;
	float					m_spacing_between;

	bool					m_pad_handling_enabled;
	bool					m_allow_wrap;

	int						m_current_grid_index; // of selected item, set to -1 when not applicable
												  // not necessarily the same as the grid index stored
												  // in the child that's selected (for "remembering"
												  // the column)
};




class CVMenu : public CBaseMenu
{
public:

							CVMenu();
	virtual					~CVMenu();
};




class CHMenu : public CBaseMenu
{
public:

							CHMenu();
	virtual					~CHMenu();
};




}

#endif
