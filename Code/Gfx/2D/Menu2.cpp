#include <core/defines.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <gel/objtrack.h>
#include <gel/Event.h>
#include <gfx/2D/Menu2.h>
#include <gfx/2D/ScreenElemMan.h>
#include <sys/timer.h>

namespace Front
{




CBaseMenu::CBaseMenu()
{
    m_selected_index = -1;
    m_selected_id = 0;
	m_in_focus = false;
	m_focus_controller = 0;

	m_regular_space_val = 0.0f;
	m_padding_scale = 1.0f;
	m_spacing_between = 0.0f;

	m_internal_just_x = 0.0f;
	m_internal_just_y = 0.0f;
	
	m_pad_handling_enabled = true;
	m_allow_wrap = true;

	m_current_grid_index = -1;
}




CBaseMenu::~CBaseMenu()
{
}




void CBaseMenu::SetProperties(Script::CStruct *pProps)
{
	float just[2] = { 0.0f, 0.0f };
	if (resolve_just(pProps, CRCD(0x67e093e4,"internal_just"), just, just+1))
		SetInternalJust(just[0], just[1]);
	
	int regular_space_val = 0;
	if (pProps->GetInteger(CRCD(0xf24adedb,"regular_space_amount"), &regular_space_val))
		m_regular_space_val = (float) regular_space_val;
	
	pProps->GetFloat(CRCD(0x6d853b88,"padding_scale"), &m_padding_scale);
	
	int spacing_between = 0;
	if (pProps->GetInteger(CRCD(0x4b4e14ab,"spacing_between"), &spacing_between))
		m_spacing_between = (float) spacing_between;
		
	if (pProps->ContainsFlag(CRCD(0x4bd08bfe,"enable_pad_handling")))
		m_pad_handling_enabled = true;
	else if (pProps->ContainsFlag(CRCD(0xc22541cc,"disable_pad_handling")))
		m_pad_handling_enabled = false;

	if (pProps->ContainsFlag(CRCD(0x43a7692d,"allow_wrap")))
		m_allow_wrap = true;
	else if (pProps->ContainsFlag(CRCD(0x5dea5c64,"dont_allow_wrap")))
		m_allow_wrap = false;
	
	CScreenElement::SetProperties(pProps);
		
	return;
}




bool CBaseMenu::PassTargetedEvent(Obj::CEvent *pEvent, bool broadcast)
{	
	
	#ifdef	__NOPT_ASSERT__												 
	Obj::CSmtPtr<CBaseMenu> p = this;
	uint32	debug_id = m_id;
	#endif		
	if (!Obj::CObject::PassTargetedEvent(pEvent))
	{
		return false;
	}	
	Dbg_MsgAssert(p, ("BaseMenu (%s) deleted itself in call to PassTargetedEvent", Script::FindChecksumName(debug_id)));
		
	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
    
	//printf("VMenu receives event of type %s\n", Script::FindChecksumName(pEvent->GetType()));
	
    if (pEvent->GetType() == Obj::CEvent::TYPE_NOTIFY_CHILD_LOCK)
    {
        // The currently selected child might have been removed. Make sure we update
        // references and send new focus event if necessary
        bool need_new_focus_event = false;
        
        if (m_selected_id) 
        {
            int current_child_index;
            CScreenElementPtr p_current_child = GetChildById(m_selected_id, &current_child_index);
            
            // see if selected child has disappeared
            if (!p_current_child) 
            {
                // child disappeared, see if another one at same index
                CScreenElementPtr p_new_child = GetChildByIndex(m_selected_index);
                if (p_new_child) 
                {
                    m_selected_id = p_new_child->GetID();
					setup_tags();
					need_new_focus_event = true;
                }
                else
                {
                    // couldn't locate replacement
                    m_selected_index = -1;
					// will set m_selected_id to 0 if no focusable item can be found
					find_focusable_item(1, true, true);
					need_new_focus_event = true;
                }
            } // end if
            else
            {
                // selected child is still there, update indices
                m_selected_index = current_child_index;
				setup_tags();
            }
        } // end if (m_selected_id)
		else
		{
            // nothing selected, so find something
			find_focusable_item(1, true, true);
			need_new_focus_event = true;
		}
        
        if (need_new_focus_event && m_in_focus) 
        {
            find_focusable_item(1, true, true);
            if (m_selected_id)
			{
				Script::CStruct data;
				data.AddInteger(CRCD(0xb30d9965,"controller"), m_focus_controller);
				p_tracker->LaunchEvent(Obj::CEvent::TYPE_FOCUS, m_selected_id, Obj::CEvent::vSYSTEM_EVENT, &data);
			}
        }

		reposition();

		pEvent->MarkHandled(m_id);
    }
	if (pEvent->GetType() == Obj::CEvent::TYPE_FOCUS && !m_in_focus)
	{
		m_focus_controller = Obj::CEvent::sExtractControllerIndex(pEvent);
		
		//if (!m_is_vertical_menu)
		//	printf("Menu %s receives focus event, sending to 0x%x\n", Script::FindChecksumName(m_id), m_selected_id);

		// see if event has a child-to-select component
		int desired_grid_index = -1;
		if (pEvent->GetData() && pEvent->GetData()->GetInteger(CRCD(0xfacf9a8b,"grid_index"), &desired_grid_index))
		{
			// find the child that matches that grid index, if any
			int match_index = -1;

			m_current_grid_index = desired_grid_index;
			
			CScreenElementPtr p_child = mp_child_list;
			int current_index = 0;
			while(p_child)
			{
				int found_grid_index = -1;
				if (p_child->GetIntegerTag(CRCD(0x5b92f8dd, "tag_grid_x"), &found_grid_index))
				{
					if (found_grid_index <= desired_grid_index)
						match_index = current_index;
				}
				p_child = p_child->GetNextSibling();
				current_index++;
			} 
			
			if (match_index != -1)
			{
				m_selected_index = match_index;
				m_selected_id = 1;
			}
		}

		// see if the event has a child_id component 
		uint32 desired_child_id;
		if ( pEvent->GetData() && pEvent->GetData()->GetChecksum( CRCD(0x229d3de4,"child_id"), &desired_child_id ) )
		{
			// printf("got a desired child_id of %x\n", desired_child_id);			
			// CScreenElementPtr p_child = GetChildById( desired_child_id );
			CScreenElementPtr p_child = mp_child_list;

			int current_index = 0;
			while ( p_child )
			{
				if ( desired_child_id == p_child->GetID() )
				{
					m_selected_id = desired_child_id;
					m_selected_index = current_index;
					break;
				}
				current_index++;
				p_child = p_child->GetNextSibling();
			}
		}

		m_in_focus = true;
		if (m_selected_id) 
		{
			find_focusable_item(1, true, false);
            if (m_selected_id)
				p_tracker->LaunchEvent(Obj::CEvent::TYPE_FOCUS, m_selected_id, Obj::CEvent::vSYSTEM_EVENT, pEvent->GetData());
		}
		pEvent->MarkHandled(m_id);
	}
	if (pEvent->GetType() == Obj::CEvent::TYPE_UNFOCUS && m_in_focus)
	{
		//printf("VMenu %s receives unfocus event\n", Script::FindChecksumName(m_id));
		m_in_focus = false;
		if (m_selected_id) 
			p_tracker->LaunchEvent(Obj::CEvent::TYPE_UNFOCUS, m_selected_id);
		pEvent->MarkHandled(m_id);
	}
	if (((pEvent->GetType() == Obj::CEvent::TYPE_PAD_UP && m_is_vertical_menu) ||
		 (pEvent->GetType() == Obj::CEvent::TYPE_PAD_LEFT && !m_is_vertical_menu)) && 
		m_in_focus && m_pad_handling_enabled) 
	{
		change_selection(-1);
		pEvent->MarkHandled(m_id);
	}
	if (((pEvent->GetType() == Obj::CEvent::TYPE_PAD_DOWN && m_is_vertical_menu) ||
		 (pEvent->GetType() == Obj::CEvent::TYPE_PAD_RIGHT && !m_is_vertical_menu)) && 
		m_in_focus && m_pad_handling_enabled) 
	{
		change_selection(1);
		pEvent->MarkHandled(m_id);
	}
	return true;
}




bool CBaseMenu::UsingRegularSpacing(float &rRegularSpaceAmount)
{
	rRegularSpaceAmount = m_regular_space_val;
	return (m_regular_space_val > 0.0001f);
}




void CBaseMenu::find_focusable_item(int dir, bool include_current, bool updateGridIndex)
{
	int count = CountChildren() * 2;

	if (m_selected_index < 0)
		m_selected_index = 0;
	
	while(count)
	{
		if (!include_current)
		{
			m_selected_index += dir;
			if (m_selected_index >= CountChildren())
			{
				if (m_allow_wrap)
					m_selected_index = 0;
				else
				{
					// go back to previous item, and reverse direction
					dir = -dir;
					m_selected_index = CountChildren() - 1;
				}
			}
			else if (m_selected_index < 0)
			{
				if (m_allow_wrap)
					m_selected_index = CountChildren() - 1;
				else
				{
					// go back to previous item, and reverse direction
					dir = -dir;
					m_selected_index = 0;
				}
			}
		}
		include_current = false;

		CScreenElementPtr p_new_child = GetChildByIndex(m_selected_index);
		Dbg_Assert(p_new_child);
		m_selected_id = p_new_child->GetID(); 
		
		if (updateGridIndex)
		{
			// update grid index that we have stored for selected child
			m_current_grid_index = -1;
			p_new_child->GetIntegerTag(CRCD(0x5b92f8dd, "tag_grid_x"), &m_current_grid_index);
		}

		if ( !p_new_child->ContainsFlagTag(CRCD(0xf33a3321, "tag_not_focusable")) && 
			 !p_new_child->EventsBlocked() )
			break;
		
		count--;
	}
	
	if (count)
	{
		setup_tags();
	}
	else
	{
		// nothing selected
		m_selected_index = -1;
		m_selected_id = 0;
	}
}




void CBaseMenu::change_selection(int dir)
{
	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
	
	// if the child is also	a menu, it will have a selected index
	int grid_index_within_child	= -1;
	
	if (m_selected_id) 
	{
		CScreenElementManager* p_manager = CScreenElementManager::Instance();
		
		// figure out which of selected child's children is selected
		CScreenElement *p_child = p_manager->GetElement(m_selected_id);
		// This assert is causing an occasional crash. I can't figure out the problem, so 
		// here is the next best solution.
		//Dbg_MsgAssert(p_child, ("Couldn't find selected child %s", Script::FindChecksumName(m_selected_id)));
		if (p_child)
			p_child->GetIntegerTag(CRCD(0x8321dd71, "tag_selected_childs_grid_index"), &grid_index_within_child);

		p_tracker->LaunchEvent(Obj::CEvent::TYPE_UNFOCUS, m_selected_id);
	}

	find_focusable_item(dir, false, true);

	if (m_selected_id) 
	{
		Script::CStruct data;
		data.AddInteger("controller", m_focus_controller);

		if (grid_index_within_child != -1)
		{
			data.AddInteger("grid_index", grid_index_within_child);
			//printf("child index is %d\n", selected_index_of_child);
		}

		p_tracker->LaunchEvent(Obj::CEvent::TYPE_FOCUS, m_selected_id, Obj::CEvent::vSYSTEM_EVENT, &data);
	}
}




void CBaseMenu::setup_tags()
{
	SetChecksumTag(CRCD(0xac988ebe, "tag_selected_id"), m_selected_id);
	SetIntegerTag(CRCD(0xf2239871, "tag_selected_index"), m_selected_index);
	//printf("setup_tags(), %s\n", Script::FindChecksumName(m_id));

	SetIntegerTag(CRCD(0x8321dd71, "tag_selected_childs_grid_index"), m_current_grid_index);
}




void CBaseMenu::reposition() 
{
	float z = 0.0f; // functions as a virtual x or y
	float biggest = 0.0f; // functions as virtual 'widest' or 'highest'

	bool use_regular_space = (m_regular_space_val > 0.0001f);
	
	if (use_regular_space)
	{
		// do first pass to find out biggest element, total height of all elements
		CScreenElementPtr p_child = mp_child_list;
		while(p_child)
		{
			// virtual dimensions, 'k' in the direction that menu items are added (i.e. of 'z')
			float scaled_j;
					
			if (m_is_vertical_menu)
				scaled_j = p_child->GetBaseW() * p_child->GetScaleX();
			else
				scaled_j = p_child->GetBaseH() * p_child->GetScaleY();
			
			z += m_regular_space_val;
			
			if (scaled_j > biggest)
				biggest = scaled_j;
			
			p_child = p_child->GetNextSibling();
		}
	}
	else
	{
		// do first pass to find out biggest element, total height of all elements
		CScreenElementPtr p_child = mp_child_list;
		while(p_child)
		{
			// virtual dimensions, 'k' in the direction that menu items are added (i.e. of 'z')
			float scaled_j, scaled_k;
					
			if (m_is_vertical_menu)
			{
				scaled_j = p_child->GetBaseW() * m_padding_scale;
				scaled_k = p_child->GetBaseH() * m_padding_scale;
			}
			else
			{
				scaled_j = p_child->GetBaseH() * m_padding_scale;
				scaled_k = p_child->GetBaseW() * m_padding_scale;
			}
			
			z += scaled_k + m_spacing_between;
			
			if (scaled_j > biggest)
				biggest = scaled_j;
			
			p_child = p_child->GetNextSibling();
		} 
		z -= m_spacing_between;
	}
		
	if ((m_object_flags & vFORCED_DIMS)) 
	{
		// "grow" the menu in case of overflow (disobeying rule of above flag)
		if (m_is_vertical_menu && z > m_base_h) 
			m_base_h = z;
		else if (!m_is_vertical_menu && z > m_base_w) 
			m_base_w = z;
	}
	else
	{
		// this element is in control of own dimensions
		if (m_is_vertical_menu)
		{
			m_base_w = biggest;
			m_base_h = z;
		}
		else
		{
			m_base_w = z;
			m_base_h = biggest;
		}
	}
	compute_ul_pos(m_target_local_props);
	m_object_flags |= vNEEDS_LOCAL_POS_CALC;
	
		
	// use vertical	justification to figure out starting position
	if (m_is_vertical_menu)
		z = (m_internal_just_y + 1.0f) * (m_base_h - z) / 2.0f;
	else
		z = (m_internal_just_x + 1.0f) * (m_base_w - z) / 2.0f;
			
	if (use_regular_space)
	{
		z += m_regular_space_val / 2.0f;
		
		// now, set positions of everything
		CScreenElementPtr p_child = mp_child_list;
		while(p_child)
		{
			if (m_is_vertical_menu)
			{
				p_child->SetJust(m_internal_just_x, 0.0f);
				p_child->SetPos((m_internal_just_x + 1.0f) * m_base_w / 2.0f, z, FORCE_INSTANT);
			}
			else
			{
				p_child->SetJust(0.0f, m_internal_just_y);
				p_child->SetPos(z, (m_internal_just_y + 1.0f) * m_base_h / 2.0f, FORCE_INSTANT);
			}
			
			z += m_regular_space_val;
			p_child = p_child->GetNextSibling();
		}
	}
	else // if !use_regular_space
	{
		// now, set positions of everything
		CScreenElementPtr p_child = mp_child_list;
		while(p_child)
		{
			if (m_is_vertical_menu)
			{
				float inc_value = p_child->GetBaseH() * m_padding_scale / 2.0f;
				z += inc_value;
				
				p_child->SetJust(m_internal_just_x, 0.0f);
				p_child->SetPos((m_internal_just_x + 1.0f) * m_base_w / 2.0f, z, FORCE_INSTANT);
				
				z += inc_value;
			}
			else
			{
				float inc_value = p_child->GetBaseW() * m_padding_scale / 2.0f;
				z += inc_value;
				
				p_child->SetJust(0.0f, m_internal_just_y);
				p_child->SetPos(z, (m_internal_just_y + 1.0f) * m_base_h / 2.0f, FORCE_INSTANT);
				
				z += inc_value;
			}
			z += m_spacing_between;
			
			if (p_child->GetFlags() & vNEEDS_LOCAL_POS_CALC)
				m_object_flags |= vNEEDS_LOCAL_POS_CALC;
			
			p_child = p_child->GetNextSibling();
		}
	}
    
	if (!m_selected_id && mp_child_list && m_in_focus) 
    {
        m_selected_index = 0;
        find_focusable_item(1, true, true);

        if (m_selected_id)
		{
			Script::CStruct data;
			data.AddInteger("controller", m_focus_controller);
			Obj::CTracker* p_tracker = Obj::CTracker::Instance();
			p_tracker->LaunchEvent(Obj::CEvent::TYPE_FOCUS, m_selected_id, Obj::CEvent::vSYSTEM_EVENT, &data);
		}
    }
}




CVMenu::CVMenu()
{
	m_is_vertical_menu = true;
	SetType(CScreenElement::TYPE_VMENU);
}




CVMenu::~CVMenu()
{
}




CHMenu::CHMenu()
{
	m_is_vertical_menu = false;
	
	SetType(CScreenElement::TYPE_HMENU);
}




CHMenu::~CHMenu()
{
}




}
