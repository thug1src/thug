#include <core/defines.h>
#include <core/singleton.h>

#include <core/hashtable.h>

#include <gel/objman.h>
#include <gel/objtrack.h>
#include <gel/Event.h>

#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/component.h>

#include <gfx/2D/ScreenElemMan.h>


// start autoduck documentation
// @DOC objtrack
// @module objtrack | None
// @subindex Scripting Database
// @index script | bails

namespace Obj
{



CEventLog::CEventLog()
{
	m_next_entry = 0;
	m_wrapped = false;
	
	mp_event_type_hash_table = NULL;
	m_event_depth = 0;
	m_num_new_entries = 0;
}



CEventLog::~CEventLog()
{
	Dbg_Assert(mp_event_type_hash_table);
	delete mp_event_type_hash_table;
}



void CEventLog::AddEntry(uint32 type, uint32 target, uint32 source, uint32 script, uint32 receiverID, EOccurenceType occurenceType)
{
	if (!mp_event_type_hash_table)
	{
		mp_event_type_hash_table = new Lst::HashTable<EventType>(8);
		Script::CArray *p_event_type_array = Script::GetArray(CRCD(0x8114f90,"event_type_array"), Script::ASSERT);
		for (int i = 0; i < (int) p_event_type_array->GetSize(); i++)
		{
			strcpy(m_event_type_table[i].mName, p_event_type_array->GetString(i));
			uint32 name_crc = Script::GenerateCRC(p_event_type_array->GetString(i));
			mp_event_type_hash_table->PutItem(name_crc, &m_event_type_table[i]);
		}
	}
			
	int last_entry = m_next_entry - 1;
	if (last_entry < 0)
		last_entry = MAX_LOG_ENTRIES - 1;
	
	if (occurenceType == vUPDATE && m_table[last_entry].m_occurence_type == vUPDATE)
	{
		// no need to keep piling on update entries, just increase tick count
		m_table[last_entry].m_tick_count++;
	}
	else
	{
		if (occurenceType == vHANDLED)
			m_event_depth--;

		Entry *p_entry = m_table + m_next_entry;
		if (occurenceType == vUPDATE)
			p_entry->m_tick_count		= 0;
		else
			p_entry->m_type 			= type;
		p_entry->m_target 				= target;
		p_entry->m_source 				= source;
		p_entry->m_script 				= script;
		p_entry->m_receiver_id			= receiverID;
		p_entry->m_occurence_type		= occurenceType;
		p_entry->m_depth				= m_event_depth;
	
		m_next_entry++;
		if (m_next_entry == MAX_LOG_ENTRIES)
		{
			m_next_entry = 0;
			m_wrapped = true;
		}
		
		if (occurenceType == vLAUNCHED)
			m_event_depth++;
		
		m_num_new_entries++;
		
		//PrintLatestEntry();
	}
}



void CEventLog::Print(bool onlyPrintNewEntries, int maxEntriesToPrint)
{
	int index = 0;
	int count = m_next_entry;
	if (m_wrapped)
	{
		index = m_next_entry;
		count = MAX_LOG_ENTRIES;
	}

	if (onlyPrintNewEntries)
	{
		maxEntriesToPrint = m_num_new_entries;
	}
	
	printf("========================================================\n");
	if (onlyPrintNewEntries)
		printf("PRINTING EVENT LOG (ONLY NEW ENTRIES)\n\n");
	else
		printf("PRINTING EVENT LOG\n\n");
	
	while(count > 0)
	{
		if (count <= maxEntriesToPrint)		
			print_entry(index);
		index++;
		if (index == MAX_LOG_ENTRIES) index = 0;
		count--;
	}
	
	printf("========================================================\n");

	m_num_new_entries = 0;
}



void CEventLog::PrintLatestEntry()
{
	int last_entry = m_next_entry - 1;
	if (last_entry < 0)
		last_entry = MAX_LOG_ENTRIES - 1;
	print_entry(last_entry);
	m_num_new_entries = 0;
}



void CEventLog::print_entry(int index)
{
#ifdef __NOPT_ASSERT__
	Entry *p_entry = m_table + index;

	//printf("%d ", p_entry->m_depth);
	for (int i = 0; i < p_entry->m_depth; i++) printf(" ");
	
	// entry format: (<...> denotes optional)
	// "ACTION: type=TYPE <target=TARGET> <receiver=RECEIVER> <source=SOURCE> <script=SCRIPT>"
	// or: "ACTION: type=TYPE <target/receiver=TARGET> <source=SOURCE> <script=SCRIPT>"
	
	if (p_entry->m_occurence_type == vLAUNCHED || p_entry->m_occurence_type == vHANDLED || p_entry->m_occurence_type == vREAD) 
	{
		if (p_entry->m_occurence_type == vLAUNCHED) 
			printf("Launched event: type=%s ", get_type_name(p_entry->m_type));
		else if (p_entry->m_occurence_type == vHANDLED) 
		{
			if (p_entry->m_receiver_id == CTracker::vID_OBJECT_TRACKER)
				printf("Event expired: type=%s ", get_type_name(p_entry->m_type));
			else
				printf("Handled event: type=%s ", get_type_name(p_entry->m_type));
		}
		else if (p_entry->m_occurence_type == vREAD) 
			printf("Read event: type=%s ", get_type_name(p_entry->m_type));
		
		bool reciever_equals_target = (p_entry->m_target == p_entry->m_receiver_id);

		if (p_entry->m_target != CEvent::vSYSTEM_EVENT) 
		{
			const char *p_target_name;
			p_target_name = Script::FindChecksumName(p_entry->m_target);
			
			if (reciever_equals_target)
				printf("target/receiver=%s ", p_target_name);
			else
				printf("target=%s ", p_target_name);
		}
		if (!reciever_equals_target && p_entry->m_receiver_id != CTracker::vID_UNSPECIFIED_RECEIVER) 
		{
			const char *p_receiver_name;
			if (p_entry->m_receiver_id == CTracker::vID_SCREEN_ELEMENT_MANAGER) 
				p_receiver_name = "Screen Element Manager";
			else if (p_entry->m_receiver_id == CTracker::vID_SUSPENDED_SCRIPT) 
				p_receiver_name = "Suspended Script";
			else if (p_entry->m_receiver_id == CTracker::vID_OBJECT_TRACKER) 
				p_receiver_name = "(Killed By) Object Tracker";
			else
				p_receiver_name = Script::FindChecksumName(p_entry->m_target);
			
			printf("receiver=%s ", p_receiver_name);
		}
		if (p_entry->m_source != CEvent::vSYSTEM_EVENT) 
			printf("source=%s ", Script::FindChecksumName(p_entry->m_source));
		if (p_entry->m_script) 
			printf("script=%s ", Script::FindChecksumName(p_entry->m_script));
		printf("\n");
	}
	else if (p_entry->m_occurence_type == vOBJECT_ADD || p_entry->m_occurence_type == vOBJECT_REMOVE) 
	{
		if (p_entry->m_occurence_type == vOBJECT_ADD)
			printf("Added object: %s", Script::FindChecksumName(p_entry->m_target));
		else
			printf("Removed object: %s", Script::FindChecksumName(p_entry->m_target));
		printf("\n");
	}
	else if (p_entry->m_occurence_type == vUPDATE) 
	{
		printf("Tick, counts = %d\n", p_entry->m_tick_count);
	}
#endif  
}



const char *CEventLog::get_type_name(uint32 type)
{
	EventType *p_type_entry = mp_event_type_hash_table->GetItem(type);
	if (p_type_entry)
	{
		return p_type_entry->mName;
	}
	else
		return Script::FindChecksumName(type);
}



DefineSingletonClass( CTracker, "CObject Tracker" );



/*
	Every CObject that is registered with a CBaseManager should also be registered with CTracker.
*/
void CTracker::addObject(CObject *pObject)
{
	Dbg_MsgAssert(pObject->GetID() != 0xFFFFFFFF, ("CObject has no ID"));
	Dbg_MsgAssert(!mp_hash_table->GetItem(pObject->GetID()), ("CObject with ID %s already in tracking system", Script::FindChecksumName(pObject->GetID())));
	
	// if object ID already being used as alias, remove the alias
	CObject *p_alias_obj = mp_alias_table->GetItem(pObject->GetID());
	if (p_alias_obj)
	{
		p_alias_obj->RemoveReference();
		mp_alias_table->FlushItem(pObject->GetID());
	}
	
	mp_hash_table->PutItem(pObject->GetID(), pObject);
#ifdef __DEBUG_OBJ_MAN__
	printf("*** Added object %s to global tracker\n", Script::FindChecksumName(pObject->GetID()));
#endif

	m_event_log.AddEntry(0, pObject->GetID(), CEvent::vSYSTEM_EVENT, 0, 0, CEventLog::vOBJECT_ADD);
}




/*
	Ryan Old Comment: The 'newIdOfObjectBeingMomentarilyRemoved' parameter is set if we are just changing the ID of the object,
	which requires removing it, then adding it again. Otherwise, this parameter will be zero (the id of the skater, fool!)
	Mick New comment: now we have an additional boolean passed top indicate if the value in newIdOfObjectBeingMomentarilyRemoved
	is valid, as it might be 0, if we are changing the id of a client skater back to 0
*/

void CTracker::removeObject(CObject *pObject, uint32 newIdOfObjectBeingMomentarilyRemoved, bool momentary_removal)
{
	mp_hash_table->FlushItem(pObject->GetID());
	if (momentary_removal)
	{
		// go through all the scripts waiting on object, change the ID
		for (int i = 0; i < vMAX_SCRIPT_ENTRIES; i++)
		{
			if (m_waiting_script_tab[i].mObjectId == pObject->GetID())
			{
				m_waiting_script_tab[i].mObjectId = newIdOfObjectBeingMomentarilyRemoved;
			}
		}
	}
	else
	{
		// only remove aliases to an object if it is going away "permanently"
		remove_aliases(pObject);
	}

#ifdef __DEBUG_OBJ_MAN__
	printf("*** Removed object %s from global tracker\n", Script::FindChecksumName(pObject->GetID()));
#endif

	m_event_log.AddEntry(0, pObject->GetID(), CEvent::vSYSTEM_EVENT, 0, 0, CEventLog::vOBJECT_REMOVE);
}




/*
	Forwards the event to listeners associated with the object. Handy for forwarding global
	events to specific recipients.
	
	If no object specified, event goes to all listeners.
*/
void CTracker::forward_event_to_listeners(CEvent *pEvent, CObject *pObject)
{
	Dbg_Assert(pEvent);
	
	// Try event on all registered listeners
	// The ref counting stuff will fire an assert if an	event listener gets deleted
	CEventListener *p_listener = mp_event_listener_list;
	while (p_listener)
	{
		// prevents listener from being deleted (without an assert)
		p_listener->m_ref_count++;
		if (!pObject || p_listener->mp_object == pObject)
			p_listener->event_filter(pEvent);
		p_listener->m_ref_count--;
		p_listener = p_listener->mp_next_listener;
	}
}




/*
	When a CObject is removed from tracking, all aliases that point to it must be removed.
*/
void CTracker::remove_aliases(CObject *pObject)
{
	while(1)
	{
		//if (mp_alias_table->GetSize() == 0)
		//	break;
		
		mp_alias_table->IterateStart();
		uint32 entry_key;
		CObject *p_entry = mp_alias_table->IterateNext(&entry_key);
		while(p_entry)
		{
			if (p_entry->GetID() == pObject->GetID())
			{
				// Found match. Remove it and repeat outer loop
				p_entry->RemoveReference();
				mp_alias_table->FlushItem(entry_key);
				break;
			}
			p_entry = mp_alias_table->IterateNext(&entry_key);
		}

		if (!p_entry)
			// no more matches left, we're done
			break;
	}
}




CTracker::CTracker()
{
	mp_hash_table = new Lst::HashTable<CObject>(8);
	mp_alias_table = new Lst::HashTable<CObject>(4);
	mp_event_receiver_table = new Lst::HashTable<CEventReceiverList>(8);	
	
	m_id_seed = 0;

	mp_event_listener_list = NULL;
	m_block_event_launching = false;
	m_event_recurse_depth = 0;

	m_next_event_script = 0;

	for (int i = 0; i < vMAX_SCRIPT_ENTRIES; i++) 
		m_waiting_script_tab[i].mEventType = vDEAD_SCRIPT_ENTRY;
	
	// XXX
	m_debug = false;
}




CTracker::~CTracker()
{
	Dbg_MsgAssert(mp_hash_table->GetSize(), ("entries still in tracker"));
	delete mp_hash_table;
}




/*
	Returns a pointer to the CObject whose ID matches the one given, returns NULL if not found.
	Will return the object if it has an alias, which is used in the front end for stuff like "MainMenu"
	But is also now used for "Skater" and "Skater2"
*/
CObject *CTracker::GetObject(uint32 id)
{
	CObject *p_obj = mp_hash_table->GetItem(id);
	if (!p_obj)
	{
		p_obj = mp_alias_table->GetItem(id);
	}
	return p_obj;
}




CObject *CTracker::GetObjectByAlias(uint32 aliasId)
{
	return mp_alias_table->GetItem(aliasId);
}




void CTracker::AddAlias(uint32 alias, CObject *pObject)
{
	// make sure alias not already being used for object ID
	Dbg_MsgAssert(!mp_hash_table->GetItem(alias), ("CObject with ID %s already in tracking system", Script::FindChecksumName(alias)));
	
	// if desired alias already being used as alias, remove old one
	CObject *p_alias_obj = mp_alias_table->GetItem(alias);
	if (p_alias_obj)
	{
		p_alias_obj->RemoveReference();
		mp_alias_table->FlushItem(alias);
	}

	pObject->AddReference();
	mp_alias_table->PutItem(alias, pObject);
}




/*
	Returns an ID that is not currently being used. Pretty high probability of not
	colliding with a user-created ID.
*/
uint32 CTracker::GetFreshId()
{
	while(1)
	{
		char name_string[64];
		sprintf(name_string, "autoid%d", m_id_seed++);
		if (m_id_seed >= 1000000)
			m_id_seed = 0;
		uint32 id = Script::GenerateCRC(name_string);
		if (!mp_hash_table->GetItem(id))
		{
			return id;
		}
	}
}

#ifdef	__NOPT_ASSERT__

// Debug function to track down corruption in scripts event handler tables
// we iterate over all the event receivers, and check that the tables that
// they point to are valid

void CTracker::ValidateReceivers()
{
	
	mp_event_receiver_table->IterateStart();
	uint32 entry_key;
	CEventReceiverList *p_entry = mp_event_receiver_table->IterateNext(&entry_key);
	while(p_entry)
	{
		p_entry = mp_event_receiver_table->IterateNext(&entry_key);
		if (p_entry)
		{
				Lst::Node< Script::CScript >*p_node = p_entry->FirstItem();
				while (p_node)
				{
					Script::CScript * p_script =  p_node->GetData();
					// Got a pointer to a node, so first validate that
					// then validate the event handler table
					
#ifndef __PLAT_WN32__	// These script functions are not necessary from PC tools

					Obj::CEventHandlerTable * p_event_handler_table = p_script->GetEventHandlerTable();
					// Validate the table object
					Dbg_MsgAssert(Mem::Valid(p_event_handler_table),("Corrupt Event handler table object for event %s\n",Script::FindChecksumName(entry_key)));
					// and the table it contains
					Dbg_MsgAssert(!p_event_handler_table->m_num_entries || Mem::Valid(p_event_handler_table->mp_tab),("Corrupt Event handler table actual table for event %s\n",Script::FindChecksumName(entry_key)));

#endif

					p_node = p_node->GetNext();					
				}

			
		}
	
	}
}
#endif


// returns true if event was handled
bool CTracker::LaunchEvent(uint32 type, uint32 target, uint32 source, Script::CStruct *pData, bool broadcast)
{
//	printf("launch event, type=%s, target=0x%x, source= 0x%x, pData = %p\n", Script::FindChecksumName(type),target,source, pData);
	
//	Dbg_MsgAssert(!broadcast,("Don't use the broadcast flag yet!!!"));
	
	Dbg_MsgAssert(!m_block_event_launching, ("event launches are blocked"));
	Dbg_MsgAssert(m_event_recurse_depth < vMAX_EVENT_RECURSE_DEPTH, ("too many nested LaunchEvents -- check your scripts"));
	//printf("### launch event depth %d\n", m_event_recurse_depth);
	m_event_recurse_depth++;
	
	// log event
	m_event_log.AddEntry(type, target, source, m_next_event_script, 0, CEventLog::vLAUNCHED);
	m_next_event_script = 0;
	
	CEvent event;
	event.m_type = type;
	event.m_target_id = target;
	event.m_source_id = source;
	event.mp_data = pData;
	
	


	
	if (broadcast)
	{	
		#ifdef __SCRIPT_EVENT_TABLE__

		// broadcast to each script that has an event handler table
		CEventReceiverList *p_event_list = mp_event_receiver_table->GetItem(type);
		if (p_event_list)
		{
			int scripts = p_event_list->CountItems();
			Dbg_MsgAssert(scripts, ("Empty list for event %s",Script::FindChecksumName(type)));
			if (scripts == 1)
			{
				// Just one object, so do a quicker sending of the event
				Lst::Node< Script::CScript >*p_node =   p_event_list->FirstItem();
				Script::CScript *p_script = p_node->GetData();
				p_script->PassTargetedEvent(&event, broadcast);			
			}
			else
			{
				// For multiple scripts, we need to make a copy of the the list of scripts
				// that will receive this. Otherwise chaos ensues, as scripts change their event handlers based on events. 
				// We also use a list of smart pointers to the mp_reference in a script, so if one event handler deletes
				// another object (and hence its script)
				// then we can safely ignore that object (since it no longer exists)
				// as the pointer will be set to NULL.			
				
				Script::CScript ** pp_scripts = new Script::CScript*[scripts];
				CSmtPtr<CRefCounted> * pp_refs = new CSmtPtr<CRefCounted>[scripts];
				
				Lst::Node< Script::CScript >*p_node = p_event_list->FirstItem();
				int n=0;
				while (p_node)
				{
					pp_scripts[n] = p_node->GetData();
					CRefCounted * p_ref = &(p_node->GetData()->m_reference_counter);
					pp_refs[n] = p_ref;
					
					n++;
					p_node = p_node->GetNext();					
				}
				for (n = 0; n<scripts; n++)
				{
					if (pp_refs[n])			// Smart pointer to script reference counter might have been deleted by some other object
					{
						pp_scripts[n]->PassTargetedEvent(&event, broadcast);			
					}
				}
				delete [] pp_refs;	 
				delete [] pp_scripts;
				
				
			}
			
		}
		
		
		#else
		// broadcast to each object that has an event handler table
		CEventReceiverList *p_event_list = mp_event_receiver_table->GetItem(type);
		if (p_event_list)
		{
			int objects = p_event_list->CountItems();
			Dbg_MsgAssert(objects, ("Empty list for event %s",Script::FindChecksumName(type)));
			if (objects == 1)
			{
				// Just one object, so do a quicker sending of the event
				Lst::Node< Obj::CObject >*p_node =   p_event_list->FirstItem();
				Obj::CObject *p_obj = p_node->GetData();
				p_obj->PassTargetedEvent(&event, broadcast);			
			}
			else
			{
				// For multiple objects, we need to make a copy of the the list of objects
				// that will receive this. Otherwise chaos ensues, as objects change their event handlers based on events. 
				// We use a list of smart pointers, so if one object handler deletes another object
				// then we can safely ignore that object (since it no longer exists)
				// as the pointer will be set to NULL.			
				CObjectPtr * pp_objects = new CObjectPtr[objects];
				Lst::Node< Obj::CObject >*p_node = p_event_list->FirstItem();
				int n=0;
				while (p_node)
				{
					pp_objects[n++] = p_node->GetData();
					p_node = p_node->GetNext();
				}
				for (n = 0; n<objects; n++)
				{
					if (pp_objects[n])			// Smart pointer might have been deleted by some other object
					{
						pp_objects[n]->PassTargetedEvent(&event, broadcast);			
					}
				}
				delete [] pp_objects;
				
				
			}
			
		}
		#endif
	}
	else
	{
		
		
		// try event on all registered listeners
		forward_event_to_listeners(&event, NULL);
	
		// if event has a specific target, then send to that CObject
		if (event.m_target_id != CEvent::vSYSTEM_EVENT && !event.WasHandled())
		{
			// look for a object with the target id
			CObject *p_object = GetObject(event.m_target_id);
			if (!p_object)
			{
				// or look for a script with the target id
				Script::CScript *p_script = Script::FindSpawnedScriptWithID(event.m_target_id);
				
				if (!p_script)
				{
					m_event_log.Print(256);
					#ifdef __NOPT_ASSERT__			
					mp_hash_table->PrintContents();
					#endif
				}
				
				Dbg_MsgAssert(p_script, ("couldn't find object or script id %s", Script::FindChecksumName(event.m_target_id)));
				
				p_script->PassTargetedEvent(&event);
			}
			else
			{
				p_object->PassTargetedEvent(&event);
			}
			// p_object may have been deleted
		}
	}
	
	/* 
		See if any scripts were waiting on the event.
		
		Unblock script if type matches AND:
		-target matches
		-OR script not associated with object
		-OR a system event
	*/
	for (int i = 0; i < vMAX_SCRIPT_ENTRIES; i++)
	{
		//if (m_waiting_script_tab[i].mEventType == Script::GenerateCRC("showed_wait_message"))
		//	Ryan("   hoorah: 0x%x 0x%x\n", m_waiting_script_tab[i].mObjectId, target);

		if (m_waiting_script_tab[i].mEventType == type && 
			(m_waiting_script_tab[i].mObjectId == target || m_waiting_script_tab[i].mObjectId == 0 || target == CEvent::vSYSTEM_EVENT || broadcast))
		{			
			// We found a suspended script of the correct type and target.
			// Can remove entry.
			m_waiting_script_tab[i].mEventType = vDEAD_SCRIPT_ENTRY;

			// does the script actually exist right now?
			Script::CScript *p_script = Script::GetScriptWithUniqueId(m_waiting_script_tab[i].mScriptId);
			if (p_script)
			{
				p_script->UnBlock();
				event.MarkHandled(vID_SUSPENDED_SCRIPT, p_script->mScriptChecksum);
				//printf("unblocking script %s (id=%d)\n", Script::FindChecksumName(p_script->mScriptChecksum), m_waiting_script_tab[i].mScriptId);
				// alright, the event is handled, so leave loop
				break;
			}
			else
			{
				//printf("script to unblock doesn't exist (id=%d)\n", m_waiting_script_tab[i].mScriptId);
			}

			// the event is NOT handled
		}
	}

	if (!event.WasHandled())
		// no one handled event, mark as killed by object tracker
		LogEventHandled(&event, vID_OBJECT_TRACKER);
	
	m_event_recurse_depth--;

	return event.WasHandled();
}




// Call right before calling LaunchEvent()
void CTracker::LogEventScript(uint32 script)
{
	m_next_event_script = script;
}



void CTracker::LogEventHandled(CEvent *pEvent, uint32 receiverID, uint32 script)
{
	// log event
	m_event_log.AddEntry(pEvent->GetType(), pEvent->GetTarget(), pEvent->GetSource(), script, receiverID, CEventLog::vHANDLED);
}



void CTracker::LogEventRead(CEvent *pEvent, uint32 receiverID, uint32 script)
{
	// log event
	m_event_log.AddEntry(pEvent->GetType(), pEvent->GetTarget(), pEvent->GetSource(), script, receiverID, CEventLog::vREAD);
}



void CTracker::LogTick()
{
	m_event_log.AddEntry(0, 0, 0, 0, 0, CEventLog::vUPDATE);
}



void CTracker::PrintEventLog(bool mostRecentOnly, int maxToPrint)
{
	m_event_log.Print(mostRecentOnly, maxToPrint);
}



void CTracker::RegisterEventListener(CEventListener *pListener)
{
	Dbg_MsgAssert(!pListener->m_registered, ("listener already registered"));

	pListener->mp_next_listener = mp_event_listener_list;
	mp_event_listener_list = pListener;
	pListener->m_registered = true;
}




void CTracker::UnregisterEventListener(CEventListener *pListener)
{
	Dbg_MsgAssert(pListener->m_registered, ("listener not registered"));

	CEventListener *p_prev = NULL;
	CEventListener *p_current = mp_event_listener_list;
	while(p_current)
	{
		if (p_current == pListener)
		{
			if (p_prev)
				p_prev->mp_next_listener = pListener->mp_next_listener;
			else
				mp_event_listener_list = pListener->mp_next_listener;
			break;
		}
		
		p_prev = p_current;
		p_current = p_current->mp_next_listener;
	}
	pListener->m_registered = false;
}

// Add this object to the list of objects that are listening for
// events of this "type", so any broadcast event of this type will go directly there
#ifdef	__SCRIPT_EVENT_TABLE__		
void		CTracker::RegisterEventReceiver(uint32 type, Script::CScript *p_obj)
#else
void		CTracker::RegisterEventReceiver(uint32 type, CObject *p_obj)
#endif
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
//	printf ("Registering Event %s from Object %s\n",Script::FindChecksumName(type), Script::FindChecksumName(p_obj->GetID()));
// If there is no CEventList in the hash table for "type" then create one
	CEventReceiverList *p_event_list = mp_event_receiver_table->GetItem(type);
	if (!p_event_list)
	{
		p_event_list = new CEventReceiverList();
		mp_event_receiver_table->PutItem(type,p_event_list); 
	}
	
// Add this object to the that list, if it is not already added
	p_event_list->RegisterEventReceiverObject(p_obj);
	
	Mem::Manager::sHandle().PopContext();		
}



// Remove this object from the list of objects that are listening for
// events of this "type"
#ifdef	__SCRIPT_EVENT_TABLE__		
void		CTracker::UnregisterEventReceiver(uint32 type, Script::CScript *p_obj)
#else
void		CTracker::UnregisterEventReceiver(uint32 type, CObject *p_obj)
#endif
{
//	printf ("Unregistering Event %s from Object %s\n",Script::FindChecksumName(type), Script::FindChecksumName(p_obj->GetID()));
	// If there is a reciever list for this "type"
	CEventReceiverList *p_event_list = mp_event_receiver_table->GetItem(type);
	if (p_event_list)
	{
		// then remove the object from the CEventList
		p_event_list->UnregisterEventReceiverObject(p_obj);
	
		// If the list is empty
		if (p_event_list->IsEmpty())
		{		
			// Then remove it from the hash table
			mp_event_receiver_table->FlushItem(type);
			// and delete it
			delete p_event_list; 
		}
	}
}


CEventReceiverList::CEventReceiverList()
{

}


// Register an object with this event receiver list
// all this does is add it to the list, so events of this "type" are sent to it
// it will not be added to the same list twice (just ignores additional registers)
	
#ifdef	__SCRIPT_EVENT_TABLE__		
void		CEventReceiverList::RegisterEventReceiverObject(Script::CScript *p_script)
{
	// See if the object is already in the list, and return if it is
	Lst::Node<Script::CScript> * p_search = m_objects.FirstItem();
	while (p_search)
	{
		if (p_search->GetData() == p_script)
			return;
		p_search = p_search->GetNext();
	}
	
	// Create a new node for adding to the list
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	Lst::Node<Script::CScript>* p_node = new Lst::Node<Script::CScript>(p_script);
	Mem::Manager::sHandle().PopContext();	
	// Just add it to the tail of the list
	m_objects.AddToTail(p_node);
}
#else

void		CEventReceiverList::RegisterEventReceiverObject(CObject *p_obj)
{
	// See if the object is already in the list, and return if it is
	Lst::Node<Obj::CObject> * p_search = m_objects.FirstItem();
	while (p_search)
	{
		if (p_search->GetData() == p_obj)
			return;
		p_search = p_search->GetNext();
	}
	
	// Create a new node for adding to the list
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	Lst::Node<Obj::CObject>* p_node = new Lst::Node<Obj::CObject>(p_obj);
	Mem::Manager::sHandle().PopContext();	
	// Just add it to the tail of the list
	m_objects.AddToTail(p_node);
}
#endif

// remove an object from this list (if it is in it)
// if it is not in the list, it will just be ignored
#ifdef	__SCRIPT_EVENT_TABLE__		
void		CEventReceiverList::UnregisterEventReceiverObject(Script::CScript *p_script)
{
	// Get the node from the list

	Lst::Node<Script::CScript>* p_node = NULL;
	Lst::Node<Script::CScript> * p_search = m_objects.FirstItem();
	while (p_search)
	{
		if (p_search->GetData() == p_script)
		{
			p_node = p_search;
			break;
		}
		p_search = p_search->GetNext();
	}
	if (p_node)
	{
		
		// Just remove it from the list
		p_node->Remove();
		
		// and delete the node (might be left with an empty list, but the tracker is responsible for cleaning it up)
		delete p_node;
	}	
}
#else

void		CEventReceiverList::UnregisterEventReceiverObject(CObject *p_obj)
{
	// Get the node from the list

	Lst::Node<Obj::CObject>* p_node = NULL;
	Lst::Node<Obj::CObject> * p_search = m_objects.FirstItem();
	while (p_search)
	{
		if (p_search->GetData() == p_obj)
		{
			p_node = p_search;
			break;
		}
		p_search = p_search->GetNext();
	}
	if (p_node)
	{
		
		// Just remove it from the list
		p_node->Remove();
		
		// and delete the node (might be left with an empty list, but the tracker is responsible for cleaning it up)
		delete p_node;
	}	
}
#endif



/*
	Causes a script to be blocked until the (future) arrival of an event of the designated type.
	If the script is associated with an object AND the event has a target, then the target
	must be that object.
*/
void CTracker::SuspendScriptUntilEvent(Script::CScript *pScript, uint32 event_type)
{
	// find an unused entry

	int unused_entry_index = -1;
	int i = 0;
	for (; i < vMAX_SCRIPT_ENTRIES; i++) 
	{
		if (m_waiting_script_tab[i].mEventType == vDEAD_SCRIPT_ENTRY)
		{
			unused_entry_index = i;
			break;
		}
	}
	
	if (unused_entry_index < 0)
	{
		// hmm, no unused entries, so find entry whose script is dead
		for (i = 0; i < vMAX_SCRIPT_ENTRIES; i++) 
		{
			Script::CScript *p_script = Script::GetScriptWithUniqueId(m_waiting_script_tab[i].mScriptId);
			if (!p_script)
			{
				unused_entry_index = i;
				break;
			}
		}
	}

	if (unused_entry_index >= 0)
	{
		//printf("suspending script %s (id=%d)\n", Script::FindChecksumName(pScript->mScriptChecksum), pScript->GetUniqueId());
		m_waiting_script_tab[unused_entry_index].mScriptId = pScript->GetUniqueId();
		if (pScript->mpObject)
			m_waiting_script_tab[unused_entry_index].mObjectId = pScript->mpObject->GetID();
		else
			m_waiting_script_tab[unused_entry_index].mObjectId = 0;
		m_waiting_script_tab[unused_entry_index].mEventType = event_type;

		#ifdef __NOPT_ASSERT__
		m_waiting_script_tab[unused_entry_index].mScriptName = pScript->mScriptChecksum;
		#endif

		// suspend the script until later notice
		pScript->Block();
	}
	else
	{
		#ifdef __NOPT_ASSERT__
		printf("out of script entries, scripts still waiting for events:\n");
		for (i = 0; i < vMAX_SCRIPT_ENTRIES; i++) 
		{
			printf("   %s\n", Script::FindChecksumName(m_waiting_script_tab[i].mScriptName));
		}
		Dbg_MsgAssert(0, ("%s\nout of script entries (%d)in WaitForEvent",pScript->GetScriptInfo(),vMAX_SCRIPT_ENTRIES));
		#endif
	}
}



#ifndef __PLAT_WN32__	// These script functions are not necessary from PC tools

// @script | LaunchEvent | 
// @parm name | type | event type
// @parm structure | data | 
bool ScriptLaunchEvent(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Although events aren't necessarily tied to the Screen Element system, it is
	// convenient to enable this function to support complex ID's, for when we
	// are dealing with Screen Elements
	Front::CScreenElementManager* pManager = Front::CScreenElementManager::Instance();
	uint32 target = pManager->ResolveComplexID(pParams, CRCD(0xb990d003,"target"));
	if (target)	
	{
		if (target == CRCD(0x36b2ee74, "system"))
			target = CEvent::vSYSTEM_EVENT;			
	}
	else
	{
		if (pScript->mpObject)
			target = pScript->mpObject->GetID();
		else
			target = CEvent::vSYSTEM_EVENT;
	}
	
	uint32 source = pManager->ResolveComplexID(pParams, CRCD(0xa075808c,"source"));
	if (source)	
	{
		if (source == CRCD(0x36b2ee74, "system"))
			source = CEvent::vSYSTEM_EVENT;			
	}
	else
	{
		if (pScript->mpObject)
			source = pScript->mpObject->GetID();
		else
			source = CEvent::vSYSTEM_EVENT;
	}	
		
	Script::CStruct *pData = NULL;
	pParams->GetStructure(CRCD(0x520c0c9c,"data"), &pData);
	
	bool broadcast = pParams->ContainsFlag(CRCD(0x640e830a,"broadcast"));
	
	CTracker* p_tracker = CTracker::Instance();	
	
	uint32 type;
	if (pParams->GetChecksum(CRCD(0x7321a8d6,"type"), &type))
	{
		p_tracker->LogEventScript(pScript->mScriptChecksum);
		p_tracker->LaunchEvent(type, target, source, pData, broadcast);
	}
	else
	{
		Script::CArray* pTypes;
		if (pParams->GetArray(CRCD(0x7321a8d6,"type"), &pTypes))
		{
			Dbg_Assert(pTypes->GetType() == ESYMBOLTYPE_NAME)
			unsigned num_events = pTypes->GetSize();
			for (unsigned n = 0; n < num_events; n++)
			{
				p_tracker->LogEventScript(pScript->mScriptChecksum);
				p_tracker->LaunchEvent(pTypes->GetChecksum(n), target, source, pData, broadcast);
			}
		}
		else
		{
			Script::CStruct* pTypes;
			if (pParams->GetStructure(CRCD(0x7321a8d6,"type"), &pTypes))
			{
				for (Script::CComponent* pComp = pTypes->GetNextComponent(); pComp; pComp = pTypes->GetNextComponent(pComp))
				{
					Dbg_Assert(pComp->mType == ESYMBOLTYPE_NAME);
					p_tracker->LogEventScript(pScript->mScriptChecksum);
					p_tracker->LaunchEvent(pComp->mChecksum, target, source, pData, broadcast);
				}
			}
			else
			{
				Dbg_MsgAssert(0, ("can't launch event without type"));
			}
		}
	}
	
	return true;
}


bool ScriptWaitForEvent(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 type;
	if (!pParams->GetChecksum(CRCD(0x7321a8d6,"type"), &type))
		Dbg_MsgAssert(0, ("no event type specified"));;
	
	CTracker* p_tracker = CTracker::Instance();	
	p_tracker->SuspendScriptUntilEvent(pScript, type);

	return true;
}




bool ScriptObjectExists(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Although events aren't necessarily tied to the Screen Element system, it is
	// convenient to enable this function to support complex ID's, for when we
	// are dealing with Screen Elements
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	uint32 id = p_manager->ResolveComplexID(pParams, CRCD(0x40c698af,"id"));
	
	CTracker* p_tracker = CTracker::Instance();
	return (p_tracker->GetObject(id) != NULL);
}




bool ScriptTerminateObjectsScripts(Script::CStruct *pParams, Script::CScript *pScript)
{
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	uint32 id = p_manager->ResolveComplexID(pParams, CRCD(0x40c698af,"id"));
	
	CTracker* p_tracker = CTracker::Instance();
	CObject *p_object = p_tracker->GetObject(id);
	
	
	// Brad - the use_proper_version flag is a last minute fix at the end of THPS4.
	// StopScriptsUsingThisObject is broken, but fixing it may break other things.  The
	// proper version works as it always should have.  It is only used when the user 
	// specifically requests it.
	bool use_proper = pParams->ContainsFlag( CRCD(0xc89f3564,"use_proper_version") );
	
	// see if array of script names
	Script::CArray *p_array;
	if (pParams->GetArray(CRCD(0x22e168c1,"scripts"), &p_array))
	{
		for (uint i = 0; i < p_array->GetSize(); i++)
		{
			if ( use_proper )
				Script::StopScriptsUsingThisObject_Proper(p_object, p_array->GetChecksum(i));
			else
				Script::StopScriptsUsingThisObject(p_object, p_array->GetChecksum(i));
		}
	}
	else
	{
		uint32 script_to_stop = 0; // means 'stop all'
		pParams->GetChecksum(CRCD(0x6166f3ad,"script_name"), &script_to_stop);
		if ( use_proper )
			Script::StopScriptsUsingThisObject_Proper(p_object, script_to_stop);
		else
			Script::StopScriptsUsingThisObject(p_object, script_to_stop);
	}

	return true;
}




bool ScriptAssignAlias(Script::CStruct *pParams, Script::CScript *pScript)
{
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	uint32 id_of_original = p_manager->ResolveComplexID(pParams, CRCD(0x40c698af,"id"));
	
	CTracker* p_tracker = CTracker::Instance();	
	CObject *p_object_to_alias = p_tracker->GetObject(id_of_original);
	Dbg_Assert(p_object_to_alias);
	
	uint32 alias;
	pParams->GetChecksum(CRCD(0x1e93946b,"alias"), &alias, Script::ASSERT);

	p_tracker->AddAlias(alias, p_object_to_alias);
	
	return true;
}




bool ScriptSetObjectProperties(Script::CStruct *pParams, Script::CScript *pScript)
{
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	uint32 id_of_original = p_manager->ResolveComplexID(pParams, CRCD(0x40c698af,"id"));
	
	CTracker* p_tracker = CTracker::Instance();	
	CObject *p_object = p_tracker->GetObject(id_of_original);
	Dbg_Assert(p_object);
	
	p_object->SetProperties(pParams);
	
	return true;
}




bool ScriptPrintEventLog(Script::CStruct *pParams, Script::CScript *pScript)
{
	bool only_print_new = true;
	
	int num_to_print = CEventLog::MAX_LOG_ENTRIES;
	if (pParams->GetInteger(NONAME, &num_to_print))
		only_print_new = false;
	
	CTracker* p_tracker = CTracker::Instance();	
	p_tracker->PrintEventLog(only_print_new, num_to_print);
	return true;
}


#endif

}
