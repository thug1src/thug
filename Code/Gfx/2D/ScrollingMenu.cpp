#include <core/defines.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <gel/objtrack.h>
#include <gel/Event.h>
#include <gfx/2D/ScrollingMenu.h>
#include <gfx/2D/Menu2.h>

namespace Front
{




CBaseScrollingMenu::CBaseScrollingMenu()
{
	RegisterWithTracker(this);
	mp_the_menu = NULL;
	m_selected_element_id = 0;

	m_in_focus = false;
	m_needs_update = false;

	m_top_or_left_line = 0.0f;
	// a very large number
	m_scroll_speed = 10000.0f;

	m_num_items_to_display = -1;
	m_window_dimension = 0.0f;
	m_regular_space_amount = -1.0f;
	
	SetType(CScreenElement::TYPE_VSCROLLING_MENU);
}




CBaseScrollingMenu::~CBaseScrollingMenu()
{
}




void CBaseScrollingMenu::SetProperties(Script::CStruct *pProps)
{
	float just[2] = { 0.0f, 0.0f };
	if (resolve_just(pProps, CRCD(0x67e093e4,"internal_just"), just, just+1))
		SetInternalJust(just[0], just[1]);
	
	int scroll_speed;
	if (pProps->GetInteger(CRCD(0x9020a1d1,"scroll_speed"), &scroll_speed))
		m_scroll_speed = (float) scroll_speed;
	
	pProps->GetInteger(CRCD(0xc855511c,"num_items_to_show"), &m_num_items_to_display);

	if (pProps->ContainsFlag(CRCD(0xed889e1f,"reset_window_top")))
	{
		m_top_or_left_line = 0;
		m_needs_update = true;
	}
	else if (pProps->ContainsFlag(CRCD(0xc3a894f6,"reset_window_bottom")))
	{
		CScreenElementPtr p_child = mp_the_menu->GetLastChild();
		float element_x, element_z, element_w, element_l;
		compute_element_area(p_child, element_x, element_z, element_w, element_l);
		m_top_or_left_line = element_z + element_l - m_window_dimension;
		m_needs_update = true;
	}
	else if ( pProps->ContainsFlag( CRCD(0xa1f3b8bb,"reset_window") ) )
	{
		m_needs_update = true;
	}
	
	CScreenElement::SetProperties(pProps);
}




bool CBaseScrollingMenu::PassTargetedEvent(Obj::CEvent *pEvent, bool broadcast)
{
	
	if (!Obj::CObject::PassTargetedEvent(pEvent))
	{
		return false;
	}
	
	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
    
    if (pEvent->GetType() == Obj::CEvent::TYPE_NOTIFY_CHILD_LOCK)
    {
        // we can assume that VMenu child is in place now, find it
		get_the_menu();
		m_needs_update = true;
		update();

		pEvent->MarkHandled(m_id);
    }
	if (pEvent->GetType() == Obj::CEvent::TYPE_FOCUS)
	{
		if (!mp_the_menu)
			get_the_menu();
		
		// event is for this scrolling menu			
		if (!m_in_focus)
		{
			//printf("VMenu %s receives focus event, sending to 0x%x\n", Script::FindChecksumName(m_id), m_selected_id);
			m_in_focus = true;
			p_tracker->LaunchEvent(Obj::CEvent::TYPE_FOCUS, mp_the_menu->GetID(), Obj::CEvent::vSYSTEM_EVENT, pEvent->GetData());		
			pEvent->MarkHandled(m_id);
		}
	}
	if (pEvent->GetType() == Obj::CEvent::TYPE_UNFOCUS && m_in_focus)
	{
		get_the_menu();
		
		//printf("VMenu %s receives focus event, sending to 0x%x\n", Script::FindChecksumName(m_id), m_selected_id);
		m_in_focus = false;
		p_tracker->LaunchEvent(Obj::CEvent::TYPE_UNFOCUS, mp_the_menu->GetID());		
		pEvent->MarkHandled(m_id);
	}
	return true;
}




void CBaseScrollingMenu::pass_event_to_listener(Obj::CEvent *pEvent)
{
	//printf("VMenu receives event of type %s\n", Script::FindChecksumName(pEvent->GetType()));
	
    if (pEvent->GetType() == Obj::CEvent::TYPE_NOTIFY_CHILD_LOCK && mp_the_menu)
    {
        if (pEvent->GetTarget() == mp_the_menu->GetID())
		{
			m_needs_update = true;
			update();
		}
		
		//Obj::CTracker* p_tracker = Obj::CTracker::Instance();
		//p_tracker->PrintEventLog(true, 100);
		
		pEvent->MarkRead(m_id);
    }
	if (pEvent->GetType() == Obj::CEvent::TYPE_FOCUS)
	{
		if (!mp_the_menu)
			get_the_menu();
		
		// see if focus event is for one of contained menu's children
		CScreenElementPtr p_child = mp_the_menu->GetFirstChild();
		while(p_child)
		{
			if (p_child->GetID() == pEvent->GetTarget())
			{
				put_element_in_view(p_child);
				pEvent->MarkRead(m_id);
				break;
			}
			p_child = p_child->GetNextSibling();
		}
	}
}




void CBaseScrollingMenu::get_the_menu()
{
	CScreenElementPtr p_child = GetFirstChild();
	while(p_child)
	{
		if ((uint32) p_child->GetType() == CRCX("vmenu") && m_is_vertical)
			mp_the_menu = (CBaseMenu *) p_child.Convert();
		else if ((uint32) p_child->GetType() == CRCX("hmenu") && !m_is_vertical)
			mp_the_menu = (CBaseMenu *) p_child.Convert();
		p_child = p_child->GetNextSibling();
	}
	
	Dbg_MsgAssert(mp_the_menu || !m_is_vertical, ("could not find VMenu"));
	Dbg_MsgAssert(mp_the_menu || m_is_vertical, ("could not find HMenu"));

	float space_amount = -1.0f;
	if (mp_the_menu->UsingRegularSpacing(space_amount) && m_num_items_to_display >= 0)
	{
		m_window_dimension = space_amount * m_num_items_to_display;
		m_regular_space_amount = space_amount;
	}
	else
	{
		m_num_items_to_display = -1;
		m_window_dimension = (m_is_vertical) ? m_base_h : m_base_w;
	}
}




void CBaseScrollingMenu::put_element_in_view(const CScreenElementPtr &pElement)
{
	Dbg_Assert(mp_the_menu);
	m_selected_element_id = pElement->GetID();
	m_needs_update = true;
}




void CBaseScrollingMenu::update()
{
	CScreenElementPtr pElement = NULL;
	int element_index = 0;
	
	float center_menu_offset_up = 0.0f;
	float center_menu_offset_down = 0.0f;
	
	if ( mp_the_menu )
	{
		if ( m_selected_element_id )
			pElement = mp_the_menu->GetChildById(m_selected_element_id, &element_index);
		else
			pElement = mp_the_menu->GetChildByIndex( 0 );
	
		if ( pElement )
		{
			int num_children = mp_the_menu->CountChildren();
	
			// Good doggy. Now find out if the selection needs to be centered in the window
			if (m_regular_space_amount >= 0.0f)
			{
				if (element_index >= (m_num_items_to_display / 2))
				{
					center_menu_offset_up = m_regular_space_amount * ((m_num_items_to_display - 1) / 2);
				}
				if (element_index < num_children - (m_num_items_to_display / 2))
				{
					center_menu_offset_down = m_regular_space_amount * (m_num_items_to_display / 2);
				}
			}
		}
	}

	if (pElement)
	{	
		float menu_x, menu_y;
		mp_the_menu->SetJust(-1.0f, -1.0f);
		mp_the_menu->GetLocalPos(&menu_x, &menu_y);

		// get upper-left of element, local coordinates (in relation to VMenu)
		float element_x, element_y, element_w, element_h;
		float element_z, element_l;
		if (m_is_vertical)
		{
			compute_element_area(pElement, element_x, element_z, element_w, element_l);
		}
		else
		{
			compute_element_area(pElement, element_z, element_y, element_l, element_h);
		}
		if (m_regular_space_amount >= 0.0f) 
		{
			element_z = m_regular_space_amount * element_index;
			element_l = m_regular_space_amount;
		}
		
		// if element top is above window top
		if (element_z < m_top_or_left_line + center_menu_offset_up)
		{
			// scroll up
			m_top_or_left_line -= m_scroll_speed;
			if (m_top_or_left_line < element_z)
				m_top_or_left_line = element_z - center_menu_offset_up;
			m_needs_update = true;
		}
		// if element bottom is below window bottom
		else if (element_z + element_l > m_top_or_left_line + m_window_dimension - center_menu_offset_down)
		{
			// scroll down
			m_top_or_left_line += m_scroll_speed;
			if (m_top_or_left_line + m_window_dimension > element_z + element_l)
				m_top_or_left_line = element_z + element_l - m_window_dimension + center_menu_offset_down;
			m_needs_update = true;
		}
			
		if (m_needs_update)
		{
			if (m_is_vertical)
				mp_the_menu->SetPos(menu_x, -m_top_or_left_line, FORCE_INSTANT);
			else
				mp_the_menu->SetPos(-m_top_or_left_line, menu_y, FORCE_INSTANT);
		
			//printf("m_top_or_left_line is %.2f\n", m_top_or_left_line);
			
			// make elements visible or invisible
			element_index = 0;
			CScreenElementPtr p_child = mp_the_menu->GetFirstChild();
			while(p_child)
			{
				if (m_is_vertical)
				{
					compute_element_area(p_child, element_x, element_z, element_w, element_l);
				}
				else
				{
					compute_element_area(p_child, element_z, element_y, element_l, element_h);
				}
				if (m_regular_space_amount >= 0.0f) 
				{
					element_z = m_regular_space_amount * element_index;
					element_l = m_regular_space_amount;
					//Ryan("oog\n");
				}
				
				const float TOLERANCE = 0.0f;
				if (element_z >= m_top_or_left_line - TOLERANCE && element_z + element_l <= m_top_or_left_line + m_window_dimension + TOLERANCE)
				{
					// top is (mostly) in window, bottom is (mostly) in window
					p_child->SetAlpha(1.0f, FORCE_INSTANT);
				}
				else
				{
					p_child->SetAlpha(0.0f, FORCE_INSTANT);
				}
		
				p_child = p_child->GetNextSibling();
				element_index++;
			}
			m_needs_update = false;
		}
	} // end if pElement
}




// finds area of element in coordinates relative to this scrolling menu
void CBaseScrollingMenu::compute_element_area(const CScreenElementPtr &pElement, float &elementX, float &elementY, float &elementW, float &elementH)
{
	pElement->GetLocalPos(&elementX, &elementY);
	elementX *= mp_the_menu->GetScaleX();
	elementY *= mp_the_menu->GetScaleY();
	elementW = pElement->GetBaseW() * pElement->GetScaleX() * mp_the_menu->GetScaleX();
	elementH = pElement->GetBaseH() * pElement->GetScaleY() * mp_the_menu->GetScaleY();
	float element_just_x, element_just_y;
	pElement->GetJust(&element_just_x, &element_just_y);
	elementX -= (element_just_x + 1.0f) * elementW / 2.0f;
	elementY -= (element_just_y + 1.0f) * elementH / 2.0f;
}




CVScrollingMenu::CVScrollingMenu()
{
	m_is_vertical = true;
}




CVScrollingMenu::~CVScrollingMenu()
{
}




CHScrollingMenu::CHScrollingMenu()
{
	m_is_vertical = false;
}




CHScrollingMenu::~CHScrollingMenu()
{
}




}
