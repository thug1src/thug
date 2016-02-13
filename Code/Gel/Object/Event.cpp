#include <core/defines.h>
#include <gel/object.h>
#include <gel/object/compositeobject.h>
#include <gel/objtrack.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/component.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <gel/event.h>


#include <gfx/2D/ScreenElement2.h>
#ifdef __NOPT_ASSERT__
#include <gfx/debuggfx.h>
#endif

namespace Obj
{
	
#ifdef __NOPT_ASSERT__
extern bool DebugSkaterScripts;
#endif




CEvent::CEvent()
{
	m_flags = 0;
}




CEvent::~CEvent()
{
	if (m_flags & mCONTROLS_OWN_DATA && mp_data)
		delete mp_data;
}




void CEvent::MarkRead(uint32 receiverId, uint32 script)
{
	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
	p_tracker->LogEventRead(this, receiverId, script);
	m_flags |= mREAD;
}




void CEvent::MarkHandled(uint32 receiverId, uint32 script)
{
	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
	p_tracker->LogEventHandled(this, receiverId, script);
	m_flags |= mHANDLED;
}




int	CEvent::sExtractControllerIndex(CEvent *pEvent)
{
	int controller = 0;
	Script::CStruct *pData = pEvent->GetData();
	if (pData)
		pData->GetInteger("controller", &controller);

	return controller;
}




CEventListener::CEventListener()
{
	m_registered = false;
	m_ref_count = 0;
}




CEventListener::~CEventListener()
{
	CTracker* p_tracker = CTracker::Instance();
	p_tracker->UnregisterEventListener(this);
	Dbg_MsgAssert(m_ref_count == 0, ("Event listener still being referenced"));
}




/*
	The flags are for optimization purposes. Don't allow events that aren't
	targeted to the attached object if it isn't necessary -- this will 
	speed things up.
	
	pObject = NULL means no associated CObject, which is legal, but you must
	set mALL_ALL_EVENTS
*/
void CEventListener::RegisterWithTracker(CObject *pObject)
{
	mp_object = pObject;

	CTracker* p_tracker = CTracker::Instance();
	p_tracker->RegisterEventListener(this);
}




/*
	Called by CTracker singleton on every event listener that is registered,
	whenever CTracker receives an event.
*/
void CEventListener::event_filter(CEvent *pEvent)
{	
	if (mp_object)
	{
		#ifdef __NOPT_ASSERT__
//		if (mp_object->GetFlags() & Front::CScreenElement::vIS_SCREEN_ELEMENT)
//			((Front::CScreenElement *) mp_object.Convert())->debug_verify_integrity();
		#endif
	}
	
	// TRICKY DELETE
	// Can in theory lead to the destruction of any CObject or any listener.
	// Will assert if this listener gets destroyed
	pass_event_to_listener(pEvent);
	
	if (mp_object)
	{
		#ifdef __NOPT_ASSERT__
//		if (mp_object->GetFlags() & Front::CScreenElement::vIS_SCREEN_ELEMENT)
//			((Front::CScreenElement *) mp_object.Convert())->debug_verify_integrity();
		#endif
	}
}




CEventHandlerTable::CEventHandlerTable()
{
	m_num_entries = 0; 
	mp_tab = NULL;
	m_valid = true;
	m_in_immediate_use_counter = 0;
	m_changed = false;
}




CEventHandlerTable::~CEventHandlerTable()
{
	Dbg_Assert(!m_in_immediate_use_counter);

	
	if (mp_tab)
	{
		for (int i = 0; i < m_num_entries; i++)
		{
			if (mp_tab[i].p_params)
			{
				delete mp_tab[i].p_params;
			}
		}
		delete [] mp_tab;
	}
}


void CEventHandlerTable::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	if (mp_tab && m_num_entries)
	{
	
		Script::CArray *p_array=new Script::CArray;
		p_array->SetSizeAndType(m_num_entries,ESYMBOLTYPE_STRUCTURE);
		for (int i=0; i<m_num_entries; ++i)
		{
			if (mp_tab[i].script != vDEAD_ENTRY)
			{
				Script::CStruct *p_ex = new Script::CStruct;
				p_ex->AddChecksum(CRCD(0x7321a8d6,"Type"), mp_tab[i].type);		
				p_ex->AddChecksum(CRCD(0xd1e70f97,"Event_Script"), mp_tab[i].script);		
				p_ex->AddChecksum(CRCD(0x923fbb3a, "Group"), mp_tab[i].group);
				p_array->SetStructure(i,p_ex);
			}
			else
			{
				Script::CStruct *p_ex = new Script::CStruct;
				p_ex->AddChecksum(NO_NAME, CRCD(0xd3d5f556, "DeadEntry"));
				p_array->SetStructure(i,p_ex);
			}
		}
		p_info->AddArrayPointer(CRCD(0x3e55ff39,"mp_event_handler_table"),p_array);

	}
#endif				 
}



// Add a single event to the table
// replacing any existing event handler with the same ex type
void	CEventHandlerTable::AddEvent(uint32 ex, uint32 scr, uint32 group, bool exception, Script::CStruct *p_params)
{
	Entry *p_entry = NULL;	

// if there is no mp_tab, then we'll need to create a single entry one 
	if (!mp_tab)
	{
		m_num_entries = 1;
		mp_tab = new Entry[1];
		p_entry = mp_tab;
	}
	else
	{
	// otherwise see it it exists, and if so, then simply replace it
		int i;

		for (i = 0; i<m_num_entries; i++)
		{
			if (mp_tab[i].type == ex)
			{
				p_entry = &mp_tab[i];
				if (p_entry->p_params)
				{
					delete p_entry->p_params;
				}
				goto GOT_ENTRY;				// goto useful
			}
		}
		
		// check for empty slots, and use them	

		for (i = 0; i<m_num_entries; i++)
		{
			if (mp_tab[i].script == vDEAD_ENTRY)
			{
				p_entry = &mp_tab[i];
				goto GOT_ENTRY;				// goto useful
			}
		}
		
		// otherwise, extend the table by one entry
	
//		printf ("Allocating new table to extend by one\n");
		Entry * p_old_tab = mp_tab;
		mp_tab = new Entry[m_num_entries + 1];
		for (i=0; i<m_num_entries; i++)
		{
			mp_tab[i] = p_old_tab[i];
		}
		delete [] p_old_tab;
		p_entry = &mp_tab[m_num_entries];
		m_num_entries++;
	
	}

GOT_ENTRY:

	Dbg_MsgAssert(p_entry, ("NULL p_entry"));

	p_entry->enabled = true;
	p_entry->exception = exception;
	p_entry->group = group;
	if (p_params)
	{
//		printf ("Allocating parameters\n");
		// if params are passed, then we need to make a copy of them
		p_entry->p_params = new Script::CStruct(*p_params);
	}
	else
	{
		p_entry->p_params = NULL;  
	}
	p_entry->script = scr;
	p_entry->type = ex;

	// Need to flag it as changed, so we can break out of pass_event
	// if an event causes a change in the table (which we are iterating over)   
	m_changed = true;

}


void CEventHandlerTable::add_from_script(Script::CArray *pArray, bool replace)
{
	Dbg_Assert(pArray);

	
	int new_entries = pArray->GetSize();
	if (replace && !new_entries)
	{
		// if no new entries, and we are "replacing", then delete any existing table
		// and return 
		if (mp_tab)
		{
			for (int i = 0; i < m_num_entries; i++)
			{
				if (mp_tab[i].p_params)
				{
					delete mp_tab[i].p_params;
				}
			}
			delete [] mp_tab;
			mp_tab = NULL;
			m_num_entries = 0;
			m_changed = true;
		}
		return;
	}

/*
	// Optimization for adding a single entry
	if (new_entries == 1)
	{

		// get the type, script pair, and any params and flags
		Script::CStruct *pEventStruct = pArray->GetStructure(0);
		
		Script::CComponent *p_left = pEventStruct->GetNextComponent(NULL);
		Dbg_MsgAssert(p_left, ("missing 'type' half of event handler pair"));
		Dbg_Assert(p_left->mType == ESYMBOLTYPE_NAME);

		Script::CComponent *p_right = pEventStruct->GetNextComponent(p_left);
		Dbg_MsgAssert(p_right, ("missing 'handler' half of event handler pair"));
		Dbg_Assert(p_right->mType == ESYMBOLTYPE_NAME);

		uint32	type = p_left->mChecksum;

		Entry *p_entry = NULL;


		// if it's a "replace" entry, then we scan through to see if we already have an entry
		// then we delete it, and can use that slot
		if (replace)
		{
			for (int i = 0; i < m_num_entries; i++)
			{
				if (mp_tab[i].type == type)
				{
					mp_tab[i].script = vDEAD_ENTRY;
					// delete the original parameters, whilst (while!) we are at it
					if (mp_tab[i].p_params)
					{
						delete mp_tab[i].p_params;
						mp_tab[i].p_params = NULL;
					}
					p_entry = &mp_tab[i];
					break;
				}
			}
		}
		
		// if not got one via replace, we need to look for an empty slot
		// 95% of the time there should be one, as the table
		// will expand to the max size required, and will not shrink
		// so we only need to add slots during expansion.

		for (int i = 0; i < m_num_entries; i++)
		{
			if (mp_tab[i].script == vDEAD_ENTRY)
			{
				p_entry = &mp_tab[i];
				break;
			}
		}

		if (p_entry)
		{

			// DANG!  This is a chunk of cut and paste code from below
			// this event handler container is in serious need
			// of refactoring
			
			// Mick, if the event handler has a "params" structure
			// then add it to the entry.
			// (note, have to be careful in cleaning these up)
			Script::CStruct *p_params;
			if (pEventStruct->GetStructure(CRCD(0x7031f10c,"params"),&p_params))
			{
				printf ("Allocating Params\n");
				p_entry->p_params = new Script::CStruct(*p_params);
			}
			else
			{
				p_entry->p_params = NULL;
			}
			
			if (!pEventStruct->GetChecksum(CRCD(0x923fbb3a, "Group"), &p_entry->group))
			{
				p_entry->group = vDEFAULT_GROUP;
			}
	
			p_entry->exception = pEventStruct->ContainsFlag(CRCD(0x80367192,"Exception"));
			
			p_entry->type = p_left->mChecksum;
			p_entry->script = p_right->mChecksum;
			Dbg_MsgAssert(p_entry->script,("Adding Null script in event handler"));
			p_entry->enabled = true;
			m_changed = true;
			return;
		}
	}

*/
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());

	// create new table	(this is the one with problems with p_params)
	// the values are not initialized
	// so if it's allocated over an old one, then
	// errors might occur
	
//	printf("Allocating memory for %d new entries\n",new_entries);
	
	Entry *p_edit_tab = new Entry[m_num_entries + new_entries];
	#ifdef __NOPT_ASSERT__ 
	int first_edit_entry = m_num_entries;	
	#endif


	int	new_entry_index = 0;

	// transfer the new entries from script array into new table
	for (int i = 0; i < new_entries; i++)
	{
		// get the type, script pair, and any params and flags
		Script::CStruct *pEventStruct = pArray->GetStructure(i);
		
		Script::CComponent *p_left = pEventStruct->GetNextComponent(NULL);
		Dbg_MsgAssert(p_left, ("missing 'type' half of event handler pair"));
		Dbg_Assert(p_left->mType == ESYMBOLTYPE_NAME);

		Script::CComponent *p_right = pEventStruct->GetNextComponent(p_left);
		Dbg_MsgAssert(p_right, ("missing 'handler' half of event handler pair"));
		Dbg_Assert(p_right->mType == ESYMBOLTYPE_NAME);

		if (replace)
		{
			// remove entry from old table, if it exists in there
			remove_entry(p_left->mChecksum);
		}
		
		Entry *p_entry = p_edit_tab + new_entry_index++;

		// Mick, if the event handler has a "params" structure
		// then add it to the entry.
		// (note, have to be careful in cleaning these up)
		Script::CStruct *p_params;
		if (pEventStruct->GetStructure(CRCD(0x7031f10c,"params"),&p_params))
		{
			Dbg_MsgAssert( (i + first_edit_entry) < (m_num_entries + new_entries),
						   ( "Array overflow" ) );
			
			p_entry->p_params = new Script::CStruct(*p_params);
		}
		else
		{
			p_entry->p_params = NULL;
		}
		
		if (!pEventStruct->GetChecksum(CRCD(0x923fbb3a, "Group"), &p_entry->group))
		{
			p_entry->group = vDEFAULT_GROUP;
		}

		p_entry->exception = pEventStruct->ContainsFlag(CRCD(0x80367192,"Exception"));
		
		p_entry->type = p_left->mChecksum;
		p_entry->script = p_right->mChecksum;
		Dbg_MsgAssert(p_entry->script,("Adding Null script in event handler"));
		p_entry->enabled = true;
		m_changed = true;
	}
	
	// if member table already exists, copy it into new one, at the end
	// (append to items already added)
	// Note that some entries in the original table might have been deleted, so
	// the size of mp_tab might end up being bigger than that indicated by m_num_entries
	// but the extra at the end is just garbage.
	// This is most common when you Set individual events on
	// and already existing table that contains that event
	// (old code would have left an uninitialized gap, causing very obscure bugs)

	if (mp_tab)
	{
		for (int i = 0; i < m_num_entries; i++)
		{
			p_edit_tab[new_entry_index++] = mp_tab[i];
		}
	}
	delete [] mp_tab;  					// old table has been coped over, so we can delete it
	mp_tab = p_edit_tab;				// and make the newly constructed table the active table
	m_num_entries = new_entry_index;	// set the number of entries to the actual counted entries (not the size of the array)
	
	Mem::Manager::sHandle().PopContext();
	
}




// doesn't change the array size, just marks entry dead (and deletes p_params struct)
void CEventHandlerTable::remove_entry(uint32 type)
{
	for (int i = 0; i < m_num_entries; i++)
	{
		if (mp_tab[i].type == type)
		{
			mp_tab[i].type = 0;
			mp_tab[i].script = vDEAD_ENTRY;
			// delete the original parameters, whilst (while!) we are at it
			if (mp_tab[i].p_params)
			{
				delete mp_tab[i].p_params;
				mp_tab[i].p_params = NULL;
			}
		}
	}
	
	// Note, we don't set m_changed here, as the table has not really changed
	// just one entry has changed
	// we will set the m_changed flag if compress_table later removed this entry
	
}


// removes all entries with the given group id
// or remove them all if "all_groups" is specifed
void CEventHandlerTable::remove_group(uint32 group)
{
	for (int i = 0; i < m_num_entries; i++)
	{
		if (mp_tab[i].group == group || group == CRCD(0x8b713e0e,"all_groups"))
		{
			mp_tab[i].script = vDEAD_ENTRY;
			if (mp_tab[i].p_params)
			{
				delete mp_tab[i].p_params;
				mp_tab[i].p_params = NULL;
			}
		}
	}
}




// changes size of table, removing dead entries
void CEventHandlerTable::compress_table()
{
	if (!mp_tab) return;

	// count up dead entries
	int num_dead = 0;
	int in = 0;
	for (; in < m_num_entries; in++)
	{
		if (mp_tab[in].script == vDEAD_ENTRY)
			num_dead++;
	}

	if (num_dead == 0) return;

	int new_size = m_num_entries - num_dead;

	m_changed = true;

	// Mick - If new table has zero size, then don't allocate it	
	// just delete the old table, and set it to NULL
	if ( 0 == new_size)
	{
		for (int i = 0; i < m_num_entries; i++)
		{
			// we're about to remove an entry
			// so delete its params if necessary
			if ( mp_tab[i].p_params )
			{
				delete mp_tab[i].p_params;
				mp_tab[i].p_params = NULL;
			}
		}

		delete[]	mp_tab;
		mp_tab = NULL;
		m_num_entries = 0;
	}
	else
	{
		
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		Entry *p_new_tab = new Entry[new_size];		
		Mem::Manager::sHandle().PopContext();
		
		int out = 0;
		for (in = 0; in < m_num_entries; in++)
		{
			if (mp_tab[in].script != vDEAD_ENTRY)
			{
				p_new_tab[out++] = mp_tab[in];
			}
			else
			{
				// we're about to remove an entry
				// so delete its params if necessary
				if ( mp_tab[in].p_params )
				{
					delete mp_tab[in].p_params;
					mp_tab[in].p_params = NULL;
				}
			}
		}
		delete[] mp_tab;
		mp_tab = p_new_tab;
		m_num_entries = new_size;
	}
}




void CEventHandlerTable::set_event_enable(uint32 type, bool state)
{
	for (int i = 0; i < m_num_entries; i++)
	{
		if (mp_tab[i].type == type)
			mp_tab[i].enabled = state;
	}
}




#ifdef	__SCRIPT_EVENT_TABLE__		
void CEventHandlerTable::pass_event(CEvent *pEvent, Script::CScript *pScript, bool broadcast)
{
	// if it's a screen element, check that events aren't blocked
#ifdef __NOPT_ASSERT__
//	if ( ( pObject->GetFlags() & Front::CScreenElement::vIS_SCREEN_ELEMENT ) && 
//		 ( pObject->GetFlags() & Front::CScreenElement::vEVENTS_BLOCKED ) )
//	{
//		return;
//	}
#endif		// __NOPT_ASSERT__



	#if 1
	bool	old_m_changed = m_changed;
	m_changed = false;		
	#else	 
	// only clear the m_changed flag if we are not recursing into here
	if (!m_in_immediate_use_counter)
	{	 	
			m_changed = false;		
	}
	#endif
	
	m_in_immediate_use_counter++;

#ifndef __PLAT_WN32__
	// Need to assert that mp_tab is valid (or we have no entries in it)
	Dbg_MsgAssert(!m_num_entries || Mem::Valid(mp_tab),("Invalid event handler table for Event %s", Script::FindChecksumName(pEvent->GetType())));
#endif

	
	Entry *p_entry = mp_tab;
	for (int i = 0; i < m_num_entries; i++)
	{  
		if (p_entry->type == pEvent->GetType() && p_entry->script != vDEAD_ENTRY && p_entry->enabled)
		{
			
			Script::CScript *p_new_script = NULL;
			Script::CStruct	*p_params = NULL;
			Script::CStruct	*p_passed_params = NULL;
			
			
			
	//		printf ("%s calls %s\n",Script::FindChecksumName(p_entry->type),Script::FindChecksumName(p_entry->script));
			
			if (pEvent->GetData())
			{
				// there is event data, so copy it into a new structure
				p_params = new Script::CStruct();
				// and then merge the parameters in with it
				if (p_entry->p_params)
				{
					*p_params += *(p_entry->p_params);
				}
				*p_params += *(pEvent->GetData());
				p_passed_params = p_params;
				// using p_params like this is safe, since SpawnScript makes its own copy
			}
			else
			{
				p_passed_params = p_entry->p_params;
			}			

			if (p_entry->exception)
			{
				#ifdef __NOPT_ASSERT__
				if (DebugSkaterScripts && (pEvent->GetTarget() == 0 || pEvent->GetSource()))
				{
					printf("%d: Exception %s, Script %s, Target %s, Source %s\n",
						(int)Tmr::GetRenderFrame(),
						Script::FindChecksumName(pEvent->GetType()),
						Script::FindChecksumName(p_entry->script),
						pEvent->GetTarget() ? Script::FindChecksumName(pEvent->GetTarget()) : "Skater",
						pEvent->GetSource() ? Script::FindChecksumName(pEvent->GetSource()) : "Skater"
					);
				}
				#endif
				
				
				// If it's an exception, we check to see if the object
				// has flagged an "OnExceptionRun" 

				if (pScript->GetOnExceptionScriptChecksum())
				{
					// Pass in the name of the exception so that certain exceptions can be ignored.
					Script::CStruct *pFoo=new Script::CStruct;
					pFoo->AddComponent(NONAME, ESYMBOLTYPE_NAME, (int)p_entry->exception);
					// Script::RunScript(pScript->GetOnExceptionScriptChecksum(), pFoo, pScript->mpObject );
					// Dan: interrupt instead of run
					uint32 checksum = pScript->GetOnExceptionScriptChecksum();
					pScript->SetOnExceptionScriptChecksum(0);	// clear it once it has been run
					pScript->Interrupt(checksum, pFoo);
					delete pFoo;
				}
				
				// the OnException script may alter the table
				if (p_entry->script != vDEAD_ENTRY)
				{
					// Exceptions act like a GOTO, so we just set the script we are running on to this new script
					// the object reamins the same
					pScript->SetScript(p_entry->script,p_passed_params,pScript->mpObject);
					pScript->Update();
				}
			}
			else
			{
				// Current Screen element code relies on spawned scripts
				// as they don't update their own scripts
				// but we are moving to an "Interrupt" model, so we
				// only support this "Spawned" model for the screen elements
				
				if (pScript->mpObject &&  pScript->mpObject->GetFlags() & Front::CScreenElement::vIS_SCREEN_ELEMENT)
				{
					// Normal events spawn a new script running on the same object as the current script (if any)
					p_new_script = Script::SpawnScript(p_entry->script, p_passed_params, 0, NULL);
					#ifdef __NOPT_ASSERT__
					p_new_script->SetCommentString("Spawned by CEventHandlerTable::pass_event, 1");
					#endif
					p_new_script->mpObject = pScript->mpObject;	   
					p_new_script->Update(); 
				}
				else
				{
					// Instead of spawning, just interrupt the current script
					pScript->Interrupt(p_entry->script, p_passed_params);					
				}				
			}

			if (p_params)
			{
				delete p_params;
			}
			
			Dbg_MsgAssert((*(uint32*)this) != 0x01010101,("%s\nCEventHandlerTable deleted whilst being used",p_new_script->GetScriptInfo()));
			//p_new_script->mpObject = NULL;

			// do logging
			//pEvent->MarkHandled(pObject->GetID(), p_entry->script);
			pEvent->MarkHandled(0, p_entry->script);	// receiver id not important
		}

		p_entry++;

		if (!m_valid)
		{
			// Looks like the spawned script deleted the CObject, invalidating this event handler table.
			// Exit now. If the outermost pass_event() in a recursive chain, then kill self.
			m_in_immediate_use_counter--;
			if (!m_in_immediate_use_counter)
				delete this;
			return;
		}
		
		if (m_changed)
		{
			break;
		}
		
	}

	// if we were previously flagged as changed, or some other function flagged us as changed
	// then say we have changed	
	m_changed = m_changed || old_m_changed;

	m_in_immediate_use_counter--;
}

#else

void CEventHandlerTable::pass_event(CEvent *pEvent, CObject *pObject, bool broadcast)
{
	// if it's a screen element, check that events aren't blocked
#ifdef __NOPT_ASSERT__
	if ( ( pObject->GetFlags() & Front::CScreenElement::vIS_SCREEN_ELEMENT ) )
	{
		Front::CScreenElement* pScreenElement = (Front::CScreenElement*)pObject;
		if ( pScreenElement->EventsBlocked() )
		{
			return;
		}		
	}
#endif		// __NOPT_ASSERT__

	// only clear the m_changed flag if we are not recursing into here
	if (!m_in_immediate_use_counter)
	{	 	
			m_changed = false;		
	}
	
	m_in_immediate_use_counter++;
	
	Entry *p_entry = mp_tab;
	for (int i = 0; i < m_num_entries; i++)
	{  
		 
//		if (broadcast && !p_entry->broadcast)
//		{
//			p_entry++;
//			continue;
//		}		
	
		if (p_entry->type == pEvent->GetType() && p_entry->script != vDEAD_ENTRY && p_entry->enabled)
		{
			
			Script::CScript *p_new_script = NULL;
			Script::CStruct	*p_params = NULL;
			Script::CStruct	*p_passed_params = NULL;
			
			// visual debugging, if there is a source and target, and they are different
			// then draw a line between the two objects
			#ifdef	__NOPT_ASSERT__
			if (Script::GetInteger(CRCD(0xffc8c5f8,"Display_event_arrows")))
			{
				if (pObject->GetFlags() & Obj::CObject::vCOMPOSITE)
				{
					uint32		source_id = pEvent->GetSource();
	//				printf ("Source = 0x%x  %s,  target = 0x%x, %s\n",source_id, Script::FindChecksumName(source_id),pObject->GetID(),Script::FindChecksumName(pObject->GetID()));
					if ( source_id != pObject->GetID() )
					{
						Obj::CObject * p_source_object = Obj::CTracker::Instance()->GetObject(source_id);
						if (p_source_object)
						{
	//						printf ("objects types %d, %d\n",pObject->GetType(), p_source_object->GetType());
							// and we only want to do it if they are composite object
							if (pObject->GetFlags() & Obj::CObject::vCOMPOSITE && p_source_object->GetFlags() & Obj::CObject::vCOMPOSITE)	// eek!
							{	
								Gfx::AddDebugArrow(((CCompositeObject*)p_source_object)->GetPos(),((CCompositeObject*)pObject)->GetPos(),0xff00ff,0xff00ff,200);
							}
						}
						else
						{
							printf ("No Source object found\n");
						}
					}
				}			
			}
			
			if (DebugSkaterScripts && (pObject->GetID() == 0 || pEvent->GetSource() == 0))
			{
				printf("%d: Exception %s, Script %s, Target %s, Source %s\n",
					(int)Tmr::GetRenderFrame(),
					Script::FindChecksumName(pEvent->GetType()),
					Script::FindChecksumName(p_entry->script),
					pEvent->GetTarget() ? Script::FindChecksumName(pEvent->GetTarget()) : "Skater",
					pEvent->GetSource() ? Script::FindChecksumName(pEvent->GetSource()) : "Skater"
				);
			}
			
			#endif
			
			
			if (pEvent->GetData())
			{
				// there is event data, so copy it into a new structure
				p_params = new Script::CStruct();
				// and then merge the parameters in with it
				if (p_entry->p_params)
				{
					*p_params += *(p_entry->p_params);
				}
				*p_params += *(pEvent->GetData());
				p_passed_params = p_params;
				// using p_params like this is safe, since SpawnScript makes its own copy
			}
			else
			{
				p_passed_params = p_entry->p_params;
			}			

			if (p_entry->exception)
			{
				// If it's an exception, we check to see if the object
				// has flagged an "OnExceptionRun" 


				// Ooh, we need to run a script, so better make sure mp_script points to something.
				if (pObject->GetScript()==NULL)
				{
					pObject->SetScript( new Script::CScript);
					#ifdef __NOPT_ASSERT__
					pObject->GetScript()->SetCommentString("Created in CEventHandlerTable::pass_event(...) (exception)");
					#endif
				}	
				// Now we're safe. Note: mp_script will get cleaned up by the CObject destructor.

				
				if (pObject->GetOnExceptionScriptChecksum())
				{
//					printf ("Running onException script %s\n",Script::FindChecksumName(pObject->GetOnExceptionScriptChecksum()));
					// Pass in the name of the exception so that certain exceptions can be ignored.
					Script::CStruct *pFoo=new Script::CStruct;
					pFoo->AddComponent(NONAME, ESYMBOLTYPE_NAME, (int)p_entry->exception);
					Script::RunScript(pObject->GetOnExceptionScriptChecksum(), pFoo, pObject );
					delete pFoo;
					pObject->SetOnExceptionScriptChecksum(0);	// clear it once it has been run
				}
			
				
				// An exception will act like a "goto"
				Script::CScript *p_old_script = pObject->GetScript();
//				printf("RUNNING exception %s script %s on object %s\n",Script::FindChecksumName(p_entry->type),Script::FindChecksumName(p_entry->script),Script::FindChecksumName(pObject->GetID()));
				p_old_script->SetScript(p_entry->script,p_passed_params,pObject);
				p_old_script->Update();
			}
			else
			{
				// Normal events spawn a script running on an object
//				printf("SPAWNING exception %s script %s on object %s\n",Script::FindChecksumName(p_entry->type),Script::FindChecksumName(p_entry->script),Script::FindChecksumName(pObject->GetID()));
				p_new_script = Script::SpawnScript(p_entry->script, p_passed_params, 0, NULL);
				#ifdef __NOPT_ASSERT__
				p_new_script->SetCommentString("Spawned by CEventHandlerTable::pass_event, 1");
				#endif
				p_new_script->mpObject = pObject;			   
				p_new_script->Update();
			}

			if (p_params)
			{
				delete p_params;
			}
			
			Dbg_MsgAssert((*(uint32*)this) != 0x01010101,("%s\nCEventHandlerTable deleted whilst being used",p_new_script->GetScriptInfo()));
			//p_new_script->mpObject = NULL;

			// do logging
			pEvent->MarkHandled(pObject->GetID(), p_entry->script);
		}

		p_entry++;

		if (!m_valid)
		{
			// Looks like the spawned script deleted the CObject, invalidating this event handler table.
			// Exit now. If the outermost pass_event() in a recursive chain, then kill self.
			m_in_immediate_use_counter--;
			if (!m_in_immediate_use_counter)
				delete this;
			return;
		}
		
		if (m_changed)
		{
			break;
		}
		
	}

	m_in_immediate_use_counter--;
}
#endif




#ifdef	__SCRIPT_EVENT_TABLE__		
// Register all the entries in this even table with the event receiver table in the tracker
// (Hey, maybe store the script in here, allow multiple entries and do away with this table?)
void	CEventHandlerTable::register_all(Script::CScript *p_script)
{
	Entry *p_entry = mp_tab;
	for (int i = 0; i < m_num_entries; i++)
	{
		// if it's a live entry, then add it as a receiver
		if (p_entry->type && p_entry->script && p_entry->script != vDEAD_ENTRY)
		{
			Obj::CTracker::Instance()->RegisterEventReceiver(p_entry->type, p_script);
		}
		p_entry++;
	}  
}

// Flush out all references from the Event Receiver table to this object, based on the event handler table
void	CEventHandlerTable::unregister_all(Script::CScript *p_script)
{
	Entry *p_entry = mp_tab;
	for (int i = 0; i < m_num_entries; i++)
	{
		if (p_entry->type)
		{
			Obj::CTracker::Instance()->UnregisterEventReceiver(p_entry->type, p_script);
		}
		p_entry++;
	}  
}

#else

// Register all the entries in this even table with the event receiver table in the tracker
// (Hey, maybe store the script in here, allow multiple entries and do away with this table?)
void	CEventHandlerTable::register_all(CObject *p_object)
{
	Entry *p_entry = mp_tab;
	for (int i = 0; i < m_num_entries; i++)
	{
		if (p_entry->type && p_entry->script)
		{
			Obj::CTracker::Instance()->RegisterEventReceiver(p_entry->type, p_object);
		}
		p_entry++;
	}  
}

// Flush out all references from the Event Receiver table to this object, based on the event handler table
void	CEventHandlerTable::unregister_all(CObject *p_object)
{
	Entry *p_entry = mp_tab;
	for (int i = 0; i < m_num_entries; i++)
	{
		if (p_entry->type)
		{
			Obj::CTracker::Instance()->UnregisterEventReceiver(p_entry->type, p_object);
		}
		p_entry++;
	}  
}

#endif

// Dump the given table
void	CEventHandlerTable::sPrintTable ( CEventHandlerTable* table )
{
#	if !defined( __PLAT_NGC__ ) || ( defined( __PLAT_NGC__ ) && !defined( __NOPT_FINAL__ ) )
	printf("====================================\n");
	printf("Event Handler Table:\n");
	if (table)
	{
		for (int i = 0; i < table->m_num_entries; i++)
		{
			Entry* p_entry = table->mp_tab + i;
			
			printf("------------------------------------\n");
			printf("  Type     : %s\n", Script::FindChecksumName(p_entry->type));
			printf("  Script   : %s\n", Script::FindChecksumName(p_entry->script));
			printf("  Group    : %s\n", Script::FindChecksumName(p_entry->group));
			printf("  Exception: %s\n", p_entry->exception ? "yes" : "no");
		}
	}
	else
	{
		printf("  Emtpy\n");
	}
	printf("====================================\n");
#endif		// __NOPT_FINAL__
}

}
