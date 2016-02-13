#include <core/defines.h>
#include <gel/objtrack.h>
#include <gel/Event.h>
#include <gel/Scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/array.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <gfx/2D/ScreenElemMan.h>
#include <gfx/2D/TextElement.h>
#include <gfx/2D/SpriteElement.h>
#include <gfx/2D/Element3d.h>
                                          
#include <gfx/2D/Window.h>
#include <gfx/2D/Menu2.h>
#include <gfx/2D/ScrollingMenu.h>
#include <gfx/camera.h>

#include <sk/modules/skate/skate.h>
#include <sk/gamenet/gamenet.h>

// start autoduck documentation
// @DOC ScreenElemMan
// @module ScreenElemMan | None
// @subindex Scripting Database
// @index script | ScreenElemMan

namespace Front
{


	
DefineSingletonClass(CScreenElementManager, "Screen Element Manager");




CScreenElementManager::CScreenElementManager()
{
	mp_root_element = NULL;
	mp_resolve_temp = NULL;

	// register event listener
	RegisterWithTracker(NULL);

	for (int i = 0; i < NUM_FOCUS_LISTS; i++)
	{
		mp_focus_list[i] = NULL;
		m_focus_list_changed[i] = false;
	}
	for (int i = 0; i < NUM_FOCUS_NODES; i++)
	{
		m_focus_node_pool[i].mpElement = NULL;
		m_focus_node_pool[i].mpNextNode = NULL;
	}

	m_num_pad_event_types = 0;
	for (int i = 0; i < MAX_PAD_EVENT_TYPES; i++)
		m_pad_event_type_tab[i] = 0;
}




CScreenElementManager::~CScreenElementManager()
{
	Dbg_MsgAssert(!m_object_list.CountItems(), ("items still in CScreenElementManager"));
}




/*
	Set 'id' to Obj::CBaseManager::vNO_OBJECT_ID if object to receive automatic ID
*/
CScreenElementPtr CScreenElementManager::CreateElement(uint32 type, uint32 id, Script::CScriptStructure *pProps)
{
	CScreenElementPtr p_new_element = NULL;
	
	uint32 heap_crc;
	int heap_num = 0;
	bool switched = false;
	if (pProps->GetChecksum("heap", &heap_crc))
	{
		switch ( heap_crc )
		{
			case 0x477fc6de:		// topdown
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
				switched = true;
				break;
			case 0xc80bf12d:		// bottomup 
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
				switched = true;
				break;
			case 0xe37e78c5:		// script
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
				switched = true;
				break;
			case 0x9f7b7843:		// network
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());
				switched = true;
				break;
			case 0x03c84a59:		// profiler
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ProfilerHeap());
				switched = true;
				break;
			case 0x935ab858:		// debug
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
				switched = true;
				break;
			case 0x5b8ab877:		// skater
				pProps->GetInteger("heapnum", &heap_num);
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterHeap(heap_num));
				switched = true;
				break;
			case 0xeabd217b:		// skaterinfo
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());
				switched = true;
				break;
			case 0x39fb63cc:		// skatergeom
				pProps->GetInteger("heapnum", &heap_num);
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterGeomHeap(heap_num));
				switched = true;
				break;
			case 0xe3f81b18:		// internettopdown
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
				switched = true;
				break;
			case 0xbaa81175:		// internetbottomup
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
				switched = true;
				break;
			case 0x1ca1ff20:		// default (ie don't change context).
				break;
			default:	// Default = frontend
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
				switched = true;
				break;
		}
	}
	else
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		switched = true;
	}

	switch(type)
	{
		case CScreenElement::TYPE_CONTAINER_ELEMENT:
			p_new_element = new CContainerElement();
			break;
		case CScreenElement::TYPE_TEXT_ELEMENT:
			p_new_element = new CTextElement();
			break;
		case CScreenElement::TYPE_VMENU:
  			p_new_element = new CVMenu();
			break;
		case CScreenElement::TYPE_HMENU:
  			p_new_element = new CHMenu();
			break;
		case CScreenElement::TYPE_TEXT_BLOCK_ELEMENT:
  			p_new_element = new CTextBlockElement();
			break;
		case CScreenElement::TYPE_SPRITE_ELEMENT:
  			p_new_element = new CSpriteElement();
			break;
		case CScreenElement::TYPE_VSCROLLING_MENU:
  			p_new_element = new CVScrollingMenu();
			break;
		case CScreenElement::TYPE_HSCROLLING_MENU:
  			p_new_element = new CHScrollingMenu();
			break;
		case CScreenElement::TYPE_ELEMENT_3D:
			p_new_element = new CElement3d();
			break;
		case CScreenElement::TYPE_WINDOW_ELEMENT:
			p_new_element = new CWindowElement();
			break;
		default:
			Dbg_MsgAssert(0, ("unknown element type 0x%x", type));
			break;
	}

	p_new_element->SetID(id);
	RegisterObject(*p_new_element);
	
	p_new_element->SetProperties(pProps);
	p_new_element->SetMorph(pProps);
	
	if ( switched ) Mem::Manager::sHandle().PopContext();
	
	
	return p_new_element;
}




void CScreenElementManager::DestroyElement(uint32 id, ERecurse recurse, EPreserveParent preserveParent, Script::CScript *pCallingScript)
{

	
	CScreenElementPtr p_element = GetElement(id, CScreenElementManager::ASSERT);
	Dbg_Assert(p_element);
	if (recurse)
	{
		mark_element_out_of_focus(p_element);
		if (p_element->mp_parent)
			p_element->mp_parent->SetChildLockState(CScreenElement::UNLOCK);
		destroy_element_recursive(preserveParent, p_element, pCallingScript);
	}
	else
	{
		mark_element_out_of_focus(p_element);
		if (pCallingScript && p_element)
			// must disassociate script from element being destroyed
			pCallingScript->DisassociateWithObject(p_element);
		
		UnregisterObject(*p_element);
		#ifdef	__NOPT_ASSERT__	
		// Mick:  screen elements are deleted directly, so LockAssert is not applicable.
		p_element->SetLockAssertOff();
		#endif
		p_element.Kill();
	}
}




/*
	Passing in pParent = NULL will give the child no parent
*/
void CScreenElementManager::SetParent(const CScreenElementPtr &pParent, const CScreenElementPtr &pChild, CScreenElement::EPosRecalc recalculatePosition)
{
	Dbg_Assert(pChild);

	CScreenElementPtr p_current_parent = pChild->mp_parent;
	if (p_current_parent)	
		Dbg_MsgAssert(!(p_current_parent->m_object_flags & CScreenElement::vCHILD_LOCK), ("can't remove child -- locked"));
	
	// if child was root element, then seek out	a new root
	if (pParent && (mp_root_element == pChild || !mp_root_element))
	{
		mp_root_element = pParent;
		while(mp_root_element->mp_parent)
			mp_root_element = mp_root_element->mp_parent;
	}

	mark_element_out_of_focus(pChild);
	
	pChild->set_parent(pParent, recalculatePosition);
}




CScreenElementPtr CScreenElementManager::GetElement(uint32 id, EAssert assert)
{
	Dbg_MsgAssert(id > 0, ("can't use 0 as an ID"));
	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
	Obj::CObject *p_object = p_tracker->GetObject(id);
	if (assert)
		Dbg_MsgAssert(p_object, ("couldn't find screen element %s", Script::FindChecksumName(id)));
	if (p_object)
	{
		CScreenElementPtr p_element = static_cast<CScreenElement *>(p_object);
		Dbg_MsgAssert(p_element, ("%s not a screen element", Script::FindChecksumName(id)));
		return p_element;
	}
	else
		return NULL;
}




CScreenElementPtr CScreenElementManager::GetElement(Script::CStruct *pStructContainingId, uint32 IdSubStructName, EAssert assert)
{
	uint32 id = ResolveComplexID(pStructContainingId, IdSubStructName);
	#ifdef __NOPT_ASSERT__
	if ( assert )
	{
		if ( !id )
			Script::PrintContents(pStructContainingId, 2);
		Dbg_MsgAssert(id, ("could not resolve ID %s", Script::FindChecksumName(IdSubStructName)));
	}
	#endif
	if ( id )
	{
		return GetElement(id, assert);
	}
	return NULL;
}


CScreenElementPtr CScreenElementManager::GetElement(Script::CStruct *pStructContainingId, char *pIdSubStructName, EAssert assert)
{
	return GetElement(pStructContainingId,Script::GenerateCRC(pIdSubStructName),ASSERT);
}



/*
	No screen element can be destroyed during this phase
*/
void CScreenElementManager::Update()
{
	if (mp_root_element)
	{
		set_tree_lock_state(CScreenElement::LOCK);
		
		//Ryan("in CScreenElementManager::Update(), timer is %d\n", Tmr::GetTime());
		mp_root_element->UpdateProperties();
		
		set_tree_lock_state(CScreenElement::UNLOCK);
	}
}




void CScreenElementManager::SetPausedState(bool pause)
{
	// find a new parentless element to be root element
	Lst::Node<Obj::CObject> *p_node = m_object_list.FirstItem();
	while(p_node)
	{
		CScreenElementPtr p_element = static_cast<CScreenElement *>(p_node->GetData());
		Dbg_Assert(p_element);
		p_element->SetMorphPausedState(pause);
		p_node = p_node->GetNext();
	}
}




// locks/unlocks all the screen elements in the tree so they can't/can be deleted without an assert
void CScreenElementManager::set_tree_lock_state(CScreenElement::ELockState state)
{
	if (mp_root_element)
	{
		CScreenElement* p_stack[32];
		p_stack[0] = mp_root_element;
		int depth = 1;
		
		while(depth)
		{
			// pop top value off stack
			CScreenElement* p_node = p_stack[--depth];

			#ifdef __NOPT_ASSERT__
			p_node->debug_verify_integrity();
			#endif
			
			// set its lock state
			if (state == CScreenElement::LOCK)
				p_node->AddReference();
			else
			{
				// this	element may have been ADDED since the call to set_tree_lock_state(LOCK),
				// so we can't depend on it being referenced
				if (p_node->IsReferenced())
					p_node->RemoveReference();
			}
			
			Dbg_Assert(depth <= 30);
			
			// put sibling and child on stack
			if (p_node->GetNextSibling())
				p_stack[depth++] = p_node->GetNextSibling();
			if (p_node->GetFirstChild())
				p_stack[depth++] = p_node->GetFirstChild();
		}
	}
}


// Optimization - as event handling become more widespread
// this should only be called on the type of events that 
// it actually handles, which is basically the pad_event_types

void CScreenElementManager::pass_event_to_listener(Obj::CEvent *pEvent)
{
	// Fill in the array of pad event types by copying it from the global "pad_event_types" script array
	if (!m_num_pad_event_types)
	{
		Script::CArray *p_event_type_array = Script::GetArray("pad_event_types", Script::ASSERT);
		m_num_pad_event_types = p_event_type_array->GetSize();
		Dbg_MsgAssert(m_num_pad_event_types <= MAX_PAD_EVENT_TYPES, ("increase size of MAX_PAD_EVENT_TYPES"));
		for (int i = 0; i < m_num_pad_event_types; i++)
		{
			m_pad_event_type_tab[i] = p_event_type_array->GetChecksum(i);
		}
	}

	// check that the controller is bound to a skater
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	if ( !skate_mod->IsMultiplayerGame() )
	{
		if ( skate_mod->m_requested_level != CRCD( 0x9f2bafb7, "load_skateshop" ) )
		{
			CScreenElementPtr p_element = GetElement( CRCD(0x21f1f4a,"startup_menu"), CScreenElementManager::DONT_ASSERT);
			if ( !p_element )
			{
				int device_num;
				Script::CStruct* pEventData = pEvent->GetData();
				if ( pEventData && pEventData->GetInteger( CRCD(0xc9428a08,"device_num"), &device_num, Script::NO_ASSERT ) )
				{
					if ( skate_mod->m_device_server_map[0] != device_num )
					{
						// this controller isn't bound to the skater!
						return;
					}
				}
			}
		}
	}
	
	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
		
	if (pEvent->GetType() == Obj::CEvent::TYPE_FOCUS) 
	{
		uint32 focus_id = pEvent->GetTarget();
		// HACK: this assert should be there, but was removed to force a last-minute fix
		//Dbg_MsgAssert(focus_id != Obj::CEvent::vSYSTEM_EVENT, ("focus event needs specific target"));
		if (focus_id != Obj::CEvent::vSYSTEM_EVENT)
		{		
			CScreenElementPtr p_focus_element = GetElement(focus_id);
			// HACK: see above
			//Dbg_MsgAssert(p_focus_element, ("focus screen element doesn't exist"));
			if ( p_focus_element && !p_focus_element->EventsBlocked() )
			{
				int controller = Obj::CEvent::sExtractControllerIndex(pEvent);
		
				mark_element_in_focus(p_focus_element, controller);
				
				pEvent->MarkRead(Obj::CTracker::vID_SCREEN_ELEMENT_MANAGER);
			}
		}
	}
	if (pEvent->GetType() == Obj::CEvent::TYPE_UNFOCUS)
	{
		uint32 unfocus_id = pEvent->GetTarget();
		// HACK: this assert should be there, but was removed to force a last-minute fix
		//Dbg_MsgAssert(unfocus_id != Obj::CEvent::vSYSTEM_EVENT, ("unfocus event needs specific target"));
		if (unfocus_id != Obj::CEvent::vSYSTEM_EVENT)
		{		
			CScreenElementPtr p_unfocus_element = GetElement(unfocus_id);
			// HACK: see above
			//Dbg_MsgAssert(p_unfocus_element, ("unfocus screen element %s doesn't exist", Script::FindChecksumName(unfocus_id)));
			if ( p_unfocus_element && !p_unfocus_element->EventsBlocked() )
			{
				mark_element_out_of_focus(p_unfocus_element);
				
				pEvent->MarkRead(Obj::CTracker::vID_SCREEN_ELEMENT_MANAGER);
			}
		}
	}
	if (pEvent->GetType() == Obj::CEvent::TYPE_NOTIFY_CHILD_UNLOCK)
	{
		uint32 unlock_id = pEvent->GetTarget();
		CScreenElementPtr p_unlocked_element = GetElement(unlock_id);
		Dbg_MsgAssert(p_unlocked_element, ("unlock screen element %s doesn't exist", Script::FindChecksumName(unlock_id)));
		
		// mark all descendants TEMPORARILY out of focus
		mark_element_out_of_focus(p_unlocked_element, true, true);
		
		pEvent->MarkRead(Obj::CTracker::vID_SCREEN_ELEMENT_MANAGER);
	}
	if (pEvent->GetType() == Obj::CEvent::TYPE_NOTIFY_CHILD_LOCK)
	{
		uint32 lock_id = pEvent->GetTarget();
		CScreenElementPtr p_last_focus_element = GetElement(lock_id);
		Dbg_MsgAssert(p_last_focus_element, ("unlock screen element %s doesn't exist", Script::FindChecksumName(lock_id)));
		
		// restore children of this element to focus
		remark_temporarily_out_of_focus_elements(p_last_focus_element);
		
		pEvent->MarkRead(Obj::CTracker::vID_SCREEN_ELEMENT_MANAGER);
	}
	if (is_pad_event(pEvent->GetType()) &&
		pEvent->GetTarget() == Obj::CEvent::vSYSTEM_EVENT) 
	{
		bool successful_handling = false;
		
		int controller = Obj::CEvent::sExtractControllerIndex(pEvent);
		
		// forward pad events (of global type) to elements in focus 		
		FocusNode *p_node = mp_focus_list[controller];
		while(p_node)
		{
			p_node->mProcessed = false;
			p_node = p_node->mpNextNode;
		}
		  
		m_focus_list_changed[controller] = true;
		while(m_focus_list_changed[controller])
		{
			m_focus_list_changed[controller] = false;
			p_node = mp_focus_list[controller];
			while(p_node)
			{
				test_focus_node(p_node);
				
				if ( !p_node->mProcessed && !p_node->mTempOutOfFocus && !p_node->mpElement->EventsBlocked() )
				{
					Dbg_MsgAssert(p_node->mpElement,("Node in focus list has NULL mpElement, entry = %d", (int) (p_node - m_focus_node_pool)));
					//Ryan("   sending pad event %d to %s\n", controller, Script::FindChecksumName(p_node->mpElement->GetID()));
					if (p_tracker->LaunchEvent(pEvent->GetType(), p_node->mpElement->GetID(), pEvent->GetSource(), pEvent->GetData()))
					{
						successful_handling = true;
						break;
					}
					p_node->mProcessed = true;
				}

				// the event just sent may have led to the clearing of this	node (and maybe others) -- if 
				// that happens, start again
				if (m_focus_list_changed[controller])
					break;
				
				p_node = p_node->mpNextNode;
			} // end while
			
			if (successful_handling)
				break;
		} // end while
		
		if (successful_handling)
			pEvent->MarkHandled(Obj::CTracker::vID_SCREEN_ELEMENT_MANAGER);
	}
}




bool CScreenElementManager::IsComplexID(Script::CStruct *pStructContainingId, char *pIdSubStructName)
{
	Script::CStruct *p_recurse_struct = NULL;	
	return pStructContainingId->GetStructure(pIdSubStructName, &p_recurse_struct);
}




/*
	This function takes a complex ID and reduces it to a regular checksum that designates a single screen
	element. The following are examples of complex ID's:
	
	id=blah1						<-- also a regular ID
	id={blah1 child=0}				<-- first child of blah1
	id=<blah1 child={0 child=2}}	<-- third child of first child of blah1
	
	id={blah1 child=local_blah}		<-- returns child of blah1 with *LOCAL* ID local_blah
									    (can be recursed as above, or mixed and matched with indices)
	
	TO BE SUPPORTED IN FUTURE:
	
	id={blah1 parent=0}				<-- parent of blah1
	id={blah1 parent=1}				<-- grandparent of blah1
	
	id={blah1 child=all_children}	<-- all children of blah1
	
	Returns 0 if ID could not be resolved, or if nothing matches the name in pIdSubStructName
*/
uint32 CScreenElementManager::ResolveComplexID(Script::CStruct *pStructContainingId, uint32 IdSubStructName)
{
	// mp_resolve_temp will be NULL when we enter this function at the beginning of a recursion chain,
	// non-NULL when inside the chain. In the latter case, it points to the element closest to 
	// desired one (so far)

	Script::CStruct *p_recurse_struct = NULL;	
	uint32 id = 0;
	int index = -1;
	if (pStructContainingId->GetStructure(IdSubStructName, &p_recurse_struct))
	{
		// Expecting the form something={...}
		// May be recursively inside some other ID structure
		
		// grab the 'x' part of structure, where x is something={id ...}
		
		// grab the ID part of something={id ...}
		// we expect either this or a child index (see below)
		// 
		uint32 contained_id = 0;
		p_recurse_struct->GetChecksum(NONAME, &contained_id);

		// grab	the child index part of something={child_index ...}
		int child_index = -1;
		p_recurse_struct->GetInteger(NONAME, &child_index);
		
		if (contained_id != 0)
		{
			if (!mp_resolve_temp)
				// we're at the top level of the recursion chain
				mp_resolve_temp = GetElement(contained_id);
			else
			{
				// we're NOT at the top level of the recursion chain, so treat ID as
				// a LOCAL id
				mp_resolve_temp = get_element_by_local_id(mp_resolve_temp, contained_id);
			}
			if (!mp_resolve_temp)
				return 0;
		}
		else if (child_index != -1) 
		{
			Dbg_MsgAssert(mp_resolve_temp, ("can't map child %d of %s to anything, no parent", child_index, Script::FindChecksumName(IdSubStructName)));
			mp_resolve_temp = mp_resolve_temp->GetChildByIndex(child_index);
			if (!mp_resolve_temp)
				return 0;
		}
		else
		{
			#ifdef	__NOPT_ASSERT__
			Script::PrintContents(pStructContainingId);
			Dbg_MsgAssert(0, ("can't resolve complex ID %s, no ID or index given.  See struct above.", Script::FindChecksumName(IdSubStructName)));
			#endif
		}
		// we can expect to recurse further
		uint32 result_id = ResolveComplexID(p_recurse_struct, CRCD(0xdd4cabd6,"child"));
		mp_resolve_temp = NULL;

		return result_id;
	}
	else if (pStructContainingId->GetChecksum(IdSubStructName, &id))
	{
		// Expecting the form something=some_checksum
		// May be recursively inside some other ID structure
		
		if (mp_resolve_temp)
		{
			// we're NOT at the top level of the recursion chain, so treat ID as
			// a LOCAL id
			mp_resolve_temp = get_element_by_local_id(mp_resolve_temp, id);
			id = mp_resolve_temp->GetID();
		}
		else
		{
			// see if ID is really an alias
			Obj::CTracker* p_tracker = Obj::CTracker::Instance();
			Obj::CObject *p_aliased_obj = p_tracker->GetObjectByAlias(id);
			if (p_aliased_obj)
				// get real ID
				id = p_aliased_obj->GetID();
		}
		
		mp_resolve_temp = NULL;
		return id;
	}
	else if (pStructContainingId->GetInteger(IdSubStructName, &index))
	{
		// Expecting the form something=child_index
		// May be recursively inside some other ID structure
		
		Dbg_MsgAssert(mp_resolve_temp, ("can't map child %d to anything, no parent", index));
		CScreenElementPtr p_child = mp_resolve_temp->GetChildByIndex(index);
		mp_resolve_temp = NULL;
		if (p_child) 		
			return p_child->GetID();
		else
			return 0;
	}
	
	mp_resolve_temp = NULL;
	return 0;	
}


uint32 CScreenElementManager::ResolveComplexID(Script::CStruct *pStructContainingId, char *pIdSubStructName)
{

	return ResolveComplexID(pStructContainingId, Script::GenerateCRC(pIdSubStructName));	
}



void CScreenElementManager::RegisterObject ( Obj::CObject& obj )
{
	#ifdef	__NOPT_ASSERT__
	CScreenElementPtr p_element = static_cast<CScreenElement *>(&obj);
	Dbg_MsgAssert(p_element, ("object registered with ScreenElement manager not ScreenElement"));
	#endif	
						   
	CBaseManager::RegisterObject(obj);
}




void CScreenElementManager::UnregisterObject ( Obj::CObject& obj )
{
	CScreenElementPtr p_unregister_element	= static_cast<CScreenElement *>(&obj);
	Dbg_Assert(p_unregister_element);
	
	Dbg_MsgAssert(!p_unregister_element->mp_parent, ("can't unregister screen element from manager -- still has parent"));
	Dbg_MsgAssert(!p_unregister_element->GetFirstChild(), ("can't unregister screen element from manager -- still has children"));
	Dbg_MsgAssert(!p_unregister_element->GetNextSibling() && !p_unregister_element->GetPrevSibling(), 
				  ("can't unregister screen element from manager -- still has siblings"));
	
	// see if unregister element is root element
	if (p_unregister_element && mp_root_element == p_unregister_element)
	{
		mp_root_element = NULL;
		
		// find a new parentless element to be root element
		Lst::Node<Obj::CObject> *p_node = m_object_list.FirstItem();
		while(p_node)
		{
			CScreenElementPtr p_element = static_cast<CScreenElement *>(p_node->GetData());
			Dbg_Assert(p_element);
			// must have no parent, can't be element that we're unregistering
			if (!p_element->mp_parent && p_element != p_unregister_element)
			{
				mp_root_element = p_element;
				break;
			}
			p_node = p_node->GetNext();
		}
	}
	
	CBaseManager::UnregisterObject(obj);
}




void CScreenElementManager::KillObject ( Obj::CObject& obj )
{
	Dbg_MsgAssert(0, ("this virtual function not supported"));
}




Lst::Head< Obj::CObject > &CScreenElementManager::GetRefObjectList()
{
	Dbg_MsgAssert(0, ("this virtual function not supported"));
	return m_object_list;
}




void CScreenElementManager::destroy_element_recursive(EPreserveParent preserve_parent, const CScreenElementPtr &pElement, Script::CScript *pCallingScript)
{
	pElement->SetChildLockState(CScreenElement::UNLOCK);
	
	CScreenElementPtr p_child = pElement->GetFirstChild();
	while(p_child)
	{
		CScreenElementPtr p_next = p_child->GetNextSibling();
		destroy_element_recursive(DONT_PRESERVE_PARENT, p_child, pCallingScript);
		p_child = p_next;
	}

	if( !preserve_parent )
	{
		if (pCallingScript && pElement)
			// must disassociate script from element being destroyed
			pCallingScript->DisassociateWithObject(pElement);
			
		SetParent(NULL, pElement);
		UnregisterObject(*pElement);
		#ifdef	__NOPT_ASSERT__	
		// Mick:  screen elements are deleted directly, so LockAssert is not applicable.
		pElement->SetLockAssertOff();;
		#endif
		delete pElement;
	}
}




CScreenElementPtr CScreenElementManager::get_element_by_local_id(const CScreenElementPtr &pParent, uint32 desiredLocalID)
{
	uint32 crc_tag_local_id = CRCX("tag_local_id");

	CScreenElementPtr p_child = pParent->GetFirstChild();
	while(p_child)
	{
		uint32 local_id;
		if (p_child->GetChecksumTag(crc_tag_local_id, &local_id))
		{
			if (local_id == desiredLocalID)
				return p_child;
		}
		p_child = p_child->GetNextSibling();
	}
	return NULL;
}




void CScreenElementManager::mark_element_in_focus(const CScreenElementPtr &pElement, int controller)
{
	Dbg_Assert(controller >= 0 && controller <= 1);
	Dbg_Assert(pElement);
	
	FocusNode *p_prev_node = NULL;
	// If in the list for the same controller, then exit function.
	// If in the list for another controller, assert
	for (int c = 0; c < NUM_FOCUS_LISTS; c++)
	{
		FocusNode *p_node = mp_focus_list[c];
		while (p_node)
		{
			test_focus_node(p_node);
			if (p_node->mpElement == pElement)
			{
				if (controller == c) 
				{
					// already in focus, make sure in full focus
					p_node->mTempOutOfFocus = false;
					return;
				}
				else
				{
					Dbg_MsgAssert(0, ("can't change existing focus to another controller"));
				}
			}
			p_prev_node = p_node;
			p_node = p_node->mpNextNode;
		}
	}
	
	// Time to make a new focus node
	// p_prev_node will point to last element in list, or NULL if no list
	
	for (int i = 0; i < NUM_FOCUS_NODES; i++)
	{
		if (!m_focus_node_pool[i].mpElement)
		{
			//Ryan("TTT element %s marked in focus, %d\n", Script::FindChecksumName(pElement->GetID()), (int) controller);
			
			m_focus_node_pool[i].mpElement = pElement;
			m_focus_node_pool[i].mId = pElement->GetID();
			m_focus_node_pool[i].mpNextNode = NULL;
			m_focus_node_pool[i].mTempOutOfFocus = false;
			// put in list -- at end of list
			if (p_prev_node)
				p_prev_node->mpNextNode = &m_focus_node_pool[i];
			else
				mp_focus_list[controller] = &m_focus_node_pool[i];
			m_focus_list_changed[controller] = true;
			return;
		}
	}
	Dbg_Assert(0);
}




void CScreenElementManager::mark_element_out_of_focus(const CScreenElementPtr &pElement, bool onlyChildren, bool tempOnly)
{
	Dbg_Assert(pElement);
	
	for (int c = 0; c < NUM_FOCUS_LISTS; c++)
	{
		// remove from list all elements that are descendents of pElement, and possibly pElement itself

		FocusNode *p_prev_node = NULL;
		FocusNode *p_node = mp_focus_list[c];
		while(p_node)
		{
			test_focus_node(p_node);
			FocusNode *p_next_node = p_node->mpNextNode;

			bool am_descendent = false;
			CScreenElementPtr p_elem = p_node->mpElement;
			if (onlyChildren && p_elem)
				p_elem = p_elem->mp_parent;
			while(p_elem)
			{
				if (p_elem == pElement)
				{
					am_descendent = true;
					break;
				}
				p_elem = p_elem->mp_parent;
			}
			
			if (am_descendent)
			{
				if (tempOnly)
				{
					// just mark temporarily out of focus
					p_node->mTempOutOfFocus = true;
					//Ryan("TTT element %s marked TEMPORARILY out of focus, %d\n", Script::FindChecksumName(p_node->mpElement->GetID()), c);
				}
				else
				{
					// found a match -- remove from list
					if (p_prev_node)
						p_prev_node->mpNextNode = p_next_node;
					else
						mp_focus_list[c] = p_next_node;
					//Ryan("TTT element %s marked OUT OF focus, %d\n", Script::FindChecksumName(p_node->mpElement->GetID()), (int) c);
					// remove from pool
					p_node->mpElement = NULL;
					m_focus_list_changed[c] = true;

					p_node = p_prev_node;
				}
			}
			
			p_prev_node = p_node;
			p_node = p_next_node;
		} // end while p_node
	} // end for c
}




// Elements temporarily marked out-of-focus are put back in focus. Only elements that are descendents 
// of pElement (or pElement) itself are restored.
void CScreenElementManager::remark_temporarily_out_of_focus_elements(const CScreenElementPtr &pElement)
{
	for (int c = 0; c < NUM_FOCUS_LISTS; c++)
	{
		FocusNode *p_node = mp_focus_list[c];
		while(p_node)
		{
			if (p_node->mTempOutOfFocus)
			{				
				// element still exists, make sure it or its ancestor is pElement
				test_focus_node(p_node);
				CScreenElementPtr p_element = p_node->mpElement;
				while (p_element)
				{
					if (p_element == pElement)
					{
						p_node->mTempOutOfFocus = false;
						break;
					}
					p_element = p_element->mp_parent;
				}
			}			
			
			p_node = p_node->mpNextNode;
		}
	} // end for c
}




void CScreenElementManager::test_focus_node(FocusNode *pNode)
{
	#ifdef __NOPT_ASSERT__
	// if this element was removed, its node should have been, too
	Dbg_MsgAssert(pNode->mpElement, ("this node entry shouldn't be in list"));
	Dbg_MsgAssert(GetElement(pNode->mId), ("ID of node associated with non-existant element")); 
	Dbg_MsgAssert(GetElement(pNode->mId) == pNode->mpElement, ("ID of node associated with element other than one pointed to"));
	#endif
}




bool CScreenElementManager::is_pad_event(uint32 eventType)
{
	for (int i = 0; i < m_num_pad_event_types; i++)
	{
		if (eventType == m_pad_event_type_tab[i])
			return true;
	}
	return false;
}

void CScreenElementManager::SetRootScreenElement( uint32 id )
{
	CScreenElementPtr p_elem = GetElement( id, ASSERT );
	mp_root_element = p_elem;
}


bool ScriptCreateScreenElement(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 type;
	if (!pParams->GetChecksum("type", &type))
		Dbg_MsgAssert(0, ("can't create screen element without type"));

	uint32 id = Obj::CBaseManager::vNO_OBJECT_ID;    
    pParams->GetChecksum(CRCD(0x40c698af,"id"), &id);

    // get id of ScreenElement
    CScreenElementManager* pManager = CScreenElementManager::Instance();
    CScreenElementPtr p_Element = pManager->CreateElement(type, id, pParams);
    id = p_Element->GetID();

    // add id to params - will overwrite if id present
    pScript->GetParams()->AddChecksum(CRCD(0x40c698af,"id"), id);

	return true;
}




bool ScriptDestroyScreenElement(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	uint32 id = pManager->ResolveComplexID(pParams, CRCD(0x40c698af,"id"));
	Dbg_MsgAssert(id, ("\n%s\nelement not in manager",pScript->GetScriptInfo()));
	
	// Let's always destroy recursively -- is there any reason not to?
	CScreenElementManager::ERecurse recurse = 
		CScreenElementManager::ERecurse(!pParams->ContainsFlag("dont_recurse"));
	CScreenElementManager::EPreserveParent preserve_parent = 
		CScreenElementManager::EPreserveParent(pParams->ContainsFlag("preserve_parent"));
	
	// This combination doesn't really make sense
	// Warn the user and do nothing since, in effect, it is an empty operation
	if( !recurse && preserve_parent )
	{
		Dbg_Printf( "Warning: Destroying an element without recursion and with preserve_parent makes no sense\n" );
		return true;
	}

	pManager->DestroyElement(id, recurse, preserve_parent, pScript);
	
	return true;
}




bool ScriptRunScriptOnScreenElement(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement(pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT);
		
	uint32 script;
	pParams->GetChecksum(NONAME, &script, true);
	
	uint32 callback = 0;
	pParams->GetChecksum(CRCD(0x86068bd9,"callback"), &callback);
    
    Script::CStruct *p_ScriptParams = NULL;
    pParams->GetStructure(CRCD(0x7031f10c,"params"), &p_ScriptParams);

    Script::CStruct *p_CallbackParams = NULL;
    pParams->GetStructure(CRCD(0xe6cf88c4,"callback_params"), &p_CallbackParams);

	// K: If script is actually a member function, then call it, for Gary bless him.
	// G: God bless us, everyone!
    Script::CSymbolTableEntry *p_entry=Script::Resolve(script);
    if (p_entry && p_entry->mType==ESYMBOLTYPE_MEMBERFUNCTION)
    {
		Dbg_MsgAssert(p_elem,("NULL p_elem"));
		p_elem->CallMemberFunction(script,p_ScriptParams,pScript);
	}	
	else
	{
		Script::CScript *p_new_script = Script::SpawnScript(script,	p_ScriptParams, callback, p_CallbackParams);
		#ifdef __NOPT_ASSERT__
		p_new_script->SetCommentString("Spawned by script command RunScriptOnScreenElement");
		p_new_script->SetOriginatingScriptInfo(pScript->GetCurrentLineNumber(),pScript->mScriptChecksum);
		#endif
		
		// K: This 'if' is now required because if script is actually a cfunc, SpawnScript will have run it,
		// then returned NULL cos it did not need to create a script.
		if (p_new_script)
		{
			p_new_script->mpObject = p_elem;
			// normally, script won't be updated until next frame -- we want it NOW, motherfucker
			p_new_script->Update();
			//Script::RunScript(script, pParams, pElement);
		}	
	}
		
	return true;
}




bool ScriptSetScreenElementProps(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement(pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT);
			
	p_elem->SetProperties(pParams);
	
	return true;
}




/*
CScreenElementPtr KlaabuBaabu()
{
	return NULL;
}
*/

bool ScriptDoScreenElementMorph(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement(pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT);
			
	p_elem->SetMorph(pParams);
	
	//CScreenElementPtr arg_bat = KlaabuBaabu();
	//printf("KlaabuBaabu 0x%x\n", arg_bat.Convert());
	//printf("KlaabuBaabu 0x%x\n", KlaabuBaabu().Convert());
	//Dbg_Assert(0);
	
	return true;
}




bool ScriptSetScreenElementLock(Script::CStruct *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement(pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT);
	Dbg_MsgAssert(p_elem, ("element not in manager"));
			
	if (pParams->ContainsFlag("off"))
		p_elem->SetChildLockState(CScreenElement::UNLOCK);
	else
		p_elem->SetChildLockState(CScreenElement::LOCK);
	
	return true;
}




bool ScriptScreenElementSystemInit(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
	CWindowElement *p_window = new CWindowElement();
	Mem::Manager::sHandle().PopContext();

	p_window->SetID(Script::GenerateCRC("root_window"));

	CScreenElementManager* pManager = CScreenElementManager::Instance();
	pManager->RegisterObject(*p_window);

	return true;
}

/*
	Temporary hack functions for THPS4. Of course, you the reader are probably now
	using them for THPS8! :x
*/

void SetScoreTHPS4(char* score_text, int skater_num)
{
	Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
	CScreenElementPtr p_element = p_screen_elem_man->GetElement(Script::GenerateCRC("the_score") + skater_num );
	if (p_element)
	{
		Dbg_MsgAssert((uint32)p_element->GetType() == CRCD(0x5200dfb6, "TextElement"), ("type is 0x%x", p_element->GetType()));
		CTextElement *p_score_element = (CTextElement *) (p_element.Convert());
		
		p_score_element->SetText(score_text);
	}
}

bool ScriptGetScreenElementDims(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement(pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT);
	
	int width = (int)( p_elem->GetBaseW() * p_elem->GetScaleX() );
	int height = (int)( p_elem->GetBaseH() * p_elem->GetScaleY() );

	pScript->GetParams()->AddInteger( "width", width );
	pScript->GetParams()->AddInteger( "height", height );
	return true;
}


// @script | TextElementConcatenate | this will append the given string on the end of the 
// current text element's string
// @parm name | id | the id of the element
// @uparm "string" | the string to append
// @flag enforce_max_width | Don't allow concatenation on a text block
// element if there's no way to wrap the element and keep it within the
// maximum width.  This has no effect on TextElements.
bool ScriptTextElementConcatenate(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement( pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT );
	
	const char* p_concat_string;
	pParams->GetString( NONAME, &p_concat_string, Script::ASSERT );

	uint32 type = p_elem->GetType();
	if ( type == CRCD( 0x40d92263, "TextBlockElement" ) )
	{
		CTextBlockElement* pTextBlockElement = (CTextBlockElement*)p_elem.Convert();
		return pTextBlockElement->Concatenate( p_concat_string, pParams->ContainsFlag( CRCD( 0x27e7a420, "enforce_max_width" ) ), pParams->ContainsFlag( CRCD(0xb8c08f55,"last_line") ) );
	}
	else if ( type == CRCD( 0x5200dfb6, "TextElement" ) )
	{
		CTextElement* pTextElement = (CTextElement*)p_elem.Convert();
		return pTextElement->Concatenate( p_concat_string );
	}
	else
	{
		Dbg_MsgAssert( 0, ( "TextElementConcatenate called on type 0x%x", p_elem->GetType() ) );
		return false;
	}
}

// @script | TextElementBackspace | removes one character from the text element.  This will
// return false if the text element is already empty
// @parm name | id | the element id
bool ScriptTextElementBackspace(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement( pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT );

	uint32 type = p_elem->GetType();
	switch ( type )
	{
		case CRCC( 0x5200dfb6, "TextElement" ):
		{
			CTextElement* p_text_element = (CTextElement*)p_elem.Convert();
			return p_text_element->Backspace();
			break;
		}
		case CRCC( 0x40d92263, "TextBlockElement" ):
		{
			CTextBlockElement* p_text_block_element = (CTextBlockElement*)p_elem.Convert();
			return p_text_block_element->Backspace();
			break;
		}
		default:
			Dbg_MsgAssert( 0, ( "TextElementBackspace called on %s, which has type %x", Script::FindChecksumName( p_elem->GetID() ), p_elem->GetType() ) );
			return false;
			break;
	}
}

// @script | GetTextElementString | this returns the current string for the specified 
// text element.  The string is returned in the script's params (string).
// @parm name | id | the text element id
bool ScriptGetTextElementString(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement( pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT );
	
	bool found_text = false;
	switch ( (uint32)p_elem->GetType() )
	{
		case CRCC( 0x5200dfb6, "TextElement" ):
		{
			CTextElement* p_text_element = (CTextElement*)p_elem.Convert();
			char *p_text = p_text_element->GetText();
			if ( p_text )
			{
				pScript->GetParams()->AddString( "string", p_text );
				found_text = true;
			}
			break;
		}
		case CRCC( 0x40d92263, "TextBlockElement" ):
		{
			CTextBlockElement* p_text_block_element = (CTextBlockElement*)p_elem.Convert();
			char text[Front::MAX_EDITABLE_TEXT_BLOCK_LENGTH];
			if ( p_text_block_element->GetText( text, Front::MAX_EDITABLE_TEXT_BLOCK_LENGTH ) )
			{
				pScript->GetParams()->AddString( "string", text );
				found_text = true;
			}
			break;
		}
		default:
			Dbg_MsgAssert( 0, ( "GetScreenElementText called on type %x", p_elem->GetType() ) );
			break;
	}
	return false;
}

// @script | GetTextElementLength | returns the length of the specified text element
// in the scripts params (length)
// @parm name | id | the text element's id
bool ScriptGetTextElementLength(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement( pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT );

	uint32 type = (uint32)p_elem->GetType();
	int length = 0;
	switch ( type )
	{
		case CRCC( 0x5200dfb6, "TextElement" ):
		{
			CTextElement* p_text_element = (CTextElement*) p_elem.Convert();
			length = p_text_element->GetLength();
			break;
		}
		case CRCC( 0x40d92263, "TextBlockElement" ):
		{
			CTextBlockElement* p_text_block_element = (CTextBlockElement*) p_elem.Convert();
			length = p_text_block_element->GetLength();
			break;
		}
		default:
			Dbg_MsgAssert( 0, ("GetTextElementLength called on screen element with type 0x%x", type ) );
			return false;
			break;
	}
	pScript->GetParams()->AddInteger( "length", length );
	return true;
}

bool ScriptGetScreenElementPosition(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement(pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT);
	
	//float x = p_elem->GetAbsX();
	//float y = p_elem->GetAbsY();
	float x, y;
	p_elem->GetLocalULPos( &x, &y );

	pScript->GetParams()->AddPair( "ScreenElementPos", x, y );
	return true;
}

// @script | MenuSelectedIndexIs | returns true if the selected item in the vmenu is 
// the item specified.  Must be called with either an index, the "first" flag, or the "last" flag
// @uparmopt 1 | some index value (first item is index 0)
// @flag first | checks if the selected item is the first item
// @flag last | checks if the selected item is the last item
bool ScriptMenuSelectedIndexIs(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement(pParams, CRCD(0x40c698af,"id"), CScreenElementManager::ASSERT);

	Dbg_MsgAssert( (uint32) p_elem->GetType() == CRCD( 0x130ef802, "vmenu" ), ( "Screen element has wrong type." ) );

	CBaseMenu* p_menu = (CBaseMenu*)p_elem.Convert();

	int selected_index = p_menu->GetSelectedIndex();
	if ( selected_index == -1 )
		return false;
	
	bool rv = false;
	
	int testIndex;
	if ( pParams->GetInteger( NONAME, &testIndex, Script::NO_ASSERT ) )
		rv = ( testIndex == selected_index );
	else if ( pParams->ContainsFlag( "first" ) )
		rv = ( selected_index == 0 );
	else if ( pParams->ContainsFlag( "last" ) )
		rv = ( selected_index == ( p_menu->CountChildren() - 1 ) );
	else
		Dbg_MsgAssert( 0, ("MenuSelectedIndexIs must be called with a number, the \"first\" flag, or the \"last\" flag") );

	return rv;
}

// @script | ScreenElementExists | returns true if the given screen element exists
// @parm name | id | the id to look for...supports compound id's
bool ScriptScreenElementExists(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();
	CScreenElementPtr p_elem = pManager->GetElement( pParams, CRCD(0x40c698af,"id"), CScreenElementManager::DONT_ASSERT );
	if ( p_elem )
		return true;
	return false;
}

// @script | GetScreenElementProps | writes the props of the given screen element to the 
// calling script's params
// @parm name | id | the id of the screen element
// @flag | dont_assert
bool ScriptGetScreenElementProps( Script::CScriptStructure *pParams, Script::CScript *pScript )
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();	  
	CScreenElementPtr p_elem = NULL;
	uint32 id = pManager->ResolveComplexID(pParams, CRCD(0x40c698af,"id"));
	if ( id )
	{
		p_elem = pManager->GetElement(id, CScreenElementManager::DONT_ASSERT);
	}
	
	if ( p_elem )
	{
		p_elem->WritePropertiesToStruct( pScript->GetParams() );
		return true;
	}
	else
	{
		if ( pParams->ContainsFlag( CRCD(0x3d92465e,"dont_assert") ) )
			return false;
		Dbg_MsgAssert( 0, ( "%s\n SetScreenElementProps unable to find screen element %s",  pScript->GetScriptInfo(),  Script::FindChecksumName(id) ) );
	}
	return false;
}

bool ScriptSetRootScreenElement( Script::CStruct* pParams, Script::CScript* pScript )
{
	CScreenElementManager* pManager = CScreenElementManager::Instance();	  
	uint32 id = pManager->ResolveComplexID(pParams, CRCD(0x40c698af,"id"));
	if ( id )
	{
		pManager->SetRootScreenElement( id );
		return true;
	}
	return false;
}

void SetTimeTHPS4(int minutes, int seconds)
{
	Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
	CScreenElementPtr p_element = p_screen_elem_man->GetElement(CRCD(0xa6343cd4,"the_time"));
	if (p_element)
	{
		Dbg_MsgAssert((uint32) p_element->GetType() == CRCD(0x5200dfb6, "TextElement"), ("type is 0x%x", p_element->GetType()));
		CTextElement *p_time_element = (CTextElement *) p_element.Convert();
		
		char time_text[64];
		sprintf(time_text, "%2d:%.2d", minutes, seconds);
		
		p_time_element->SetText(time_text);
	}
}


void HideTimeTHPS4()
{
    Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
    CScreenElementPtr p_element = p_screen_elem_man->GetElement(CRCD(0xa6343cd4,"the_time"));
    if (p_element)
    {
        Dbg_MsgAssert((uint32) p_element->GetType() == CRCD(0x5200dfb6, "TextElement"), ("type is 0x%x", p_element->GetType()));
        CTextElement *p_time_element = (CTextElement *) p_element.Convert();
        p_time_element->SetText( "" );
    }
}


}

