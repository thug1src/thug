#ifndef __GEL_OBJTRACK_H
#define __GEL_OBJTRACK_H

#include <gel/event.h>

namespace Script
{
	class CStruct;
	class CScript;
}


namespace Front
{
	class CScreenElementManager;
}


namespace Lst
{
	template< class _V > class HashTable;
}


namespace Obj
{

class CObject;
//class CEvent;
class CEventListener;
	

/*
	An instance of this class is attached to the CTracker singleton. It tracks the launching
	and handling of all events in the system, for debugging purposes.
*/
class CEventLog
{
public:
	
	enum EOccurenceType
	{
		vLAUNCHED,
		vHANDLED,
		vREAD,
		vUPDATE,
		vOBJECT_ADD,
		vOBJECT_REMOVE,
	};

	enum
	{
		MAX_LOG_ENTRIES			= 1024,
		MAX_EVENT_TYPES			= 256,
	};

								CEventLog();
								~CEventLog();

	void						AddEntry(uint32 type, uint32 target, uint32 source, uint32 script, uint32 receiverID, EOccurenceType lifePoint);
	void						Print(bool onlyPrintNewEntries = true, int maxEntriesToPrint = MAX_LOG_ENTRIES);
	void						PrintLatestEntry();
	void						Clear() {m_num_new_entries = 0;}

private:

	void						print_entry(int index);
	const char *				get_type_name(uint32 type);
	
	struct Entry
	{
		union
		{
			uint32				m_type;
			int					m_tick_count; // applies to update type entries
		};
		uint32 					m_target;
		uint32 					m_source;
		uint32					m_script;					
		uint32					m_receiver_id;
		EOccurenceType			m_occurence_type;
		int						m_depth;
	};

	Entry						m_table[MAX_LOG_ENTRIES];
	int							m_next_entry;
	bool						m_wrapped;

	struct EventType
	{
		//uint32					mCrc;
		char					mName[64];
	};
	EventType					m_event_type_table[MAX_EVENT_TYPES];
	Lst::HashTable<EventType> *	mp_event_type_hash_table;

	int							m_event_depth;
	int							m_num_new_entries;
};


// CEventReceiverList comprises of an event type
// and a list of object that can receive this type of event
class	CEventReceiverList : public Spt::Class
{
public:
				CEventReceiverList();

#ifdef	__SCRIPT_EVENT_TABLE__		
	void		RegisterEventReceiverObject(Script::CScript *p_obj);
	void		UnregisterEventReceiverObject(Script::CScript *p_obj);
	bool		IsEmpty() {return m_objects.IsEmpty();};
	Lst::Node< Script::CScript >*	FirstItem() {return m_objects.FirstItem();}
	int			CountItems() {return m_objects.CountItems();}
private:
//	uint32						m_type;	   					// The type of event this is
	Lst::Head<Script::CScript>		m_objects;			// a list of objects (SCRIPTS) that are listening for it
#else
	void		RegisterEventReceiverObject(CObject *p_obj);
	void		UnregisterEventReceiverObject(CObject *p_obj);
	bool		IsEmpty() {return m_objects.IsEmpty();};
	Lst::Node< Obj::CObject >*	FirstItem() {return m_objects.FirstItem();}
	int			CountItems() {return m_objects.CountItems();}
private:
//	uint32						m_type;	   					// The type of event this is
	Lst::Head<Obj::CObject>		m_objects;			// a list of objects that are listening for it
#endif
};


/*
	See document for more info on this class: Q:\sk4\Docs\programming\Object Scripting System.htm
	
	This class keeps track of all registered CObjects, and provides functions for retreiving
	them by ID. It also takes care of launching and distributing events. It also can reactivate
	scripts that are suspended while waiting for an event.
*/
class CTracker : public Spt::Class
{
	friend class CBaseManager;
	friend class CGeneralManager;
	friend class Front::CScreenElementManager;

	DeclareSingletonClass( CTracker );

public:								
								
	// In logging
	enum ELoggingSystem
	{
		vID_UNSPECIFIED_RECEIVER				= 0,
		vID_SCREEN_ELEMENT_MANAGER				= 1,
		vID_SUSPENDED_SCRIPT					= 2,
		vID_OBJECT_TRACKER						= 3,
	};

								CTracker();
	virtual						~CTracker();

	CObject *					GetObject(uint32 id);
	
	// See the menu document for a description of aliases
	CObject *					GetObjectByAlias(uint32 aliasId);
	void						AddAlias(uint32 alias, CObject *pObject);

	uint32						GetFreshId();

	#ifdef	__NOPT_ASSERT__
	void 						ValidateReceivers();
	#endif
	

	bool						LaunchEvent(uint32 type, uint32 target = CEvent::vSYSTEM_EVENT, uint32 source = CEvent::vSYSTEM_EVENT, Script::CStruct *pData = NULL, bool broadcast=false);
	void						BlockEventLaunching(bool block) {m_block_event_launching = block;}
	
	void 						LogEventScript(uint32 script = 0);
	void						LogEventHandled(CEvent *pEvent, uint32 receiverID = 0, uint32 script = 0);
	void						LogEventRead(CEvent *pEvent, uint32 receiverID = 0, uint32 script = 0);
	void						LogTick();
	void 						PrintEventLog(bool mostRecentOnly, int maxToPrint);
	void						EmptyLog() {m_event_log.Clear();}

// Event Listener are special objects that will get sent all types of events
// and it is up to them to 	
	void 						RegisterEventListener(CEventListener *pListener);
	void						UnregisterEventListener(CEventListener *pListener);

// Event Receivers are just CObject and they only get sent events that they tell the tracker they are listening for
	
#ifdef	__SCRIPT_EVENT_TABLE__		
	void						RegisterEventReceiver(uint32 type, Script::CScript *p_obj);
	void						UnregisterEventReceiver(uint32 type, Script::CScript *p_obj);
#else	
	void						RegisterEventReceiver(uint32 type, CObject *p_obj);
	void						UnregisterEventReceiver(uint32 type, CObject *p_obj);
#endif	
	// Note, removing an object should remove all event receivers.

	void						SuspendScriptUntilEvent(Script::CScript *pScript, uint32 event_type);

private:

	void						addObject(CObject *pObject);
	void 						removeObject(CObject *pObject, uint32 newIdOfObjectBeingMomentarilyRemoved, bool momentary_removal);

	void						forward_event_to_listeners(CEvent *pEvent, CObject *pObject);

	void						remove_aliases(CObject *pObject);
	
	int							m_id_seed;
	
	Lst::HashTable<CObject> *	mp_hash_table;
	Lst::HashTable<CObject> *	mp_alias_table;

// The hash table of event listeners is keyed off the "type" of the event
// (the "type" is the actual event name, as a uint32 CRC)	
	Lst::HashTable<CEventReceiverList> *	mp_event_receiver_table;
	
	CEventListener *			mp_event_listener_list;
	
	bool						m_block_event_launching;
	enum 
	{
		vMAX_EVENT_RECURSE_DEPTH = 32,
	};
	int							m_event_recurse_depth;

	CEventLog					m_event_log;
	uint32						m_next_event_script;
	
	// Used to keep track of of scripts that are suspended while waiting for an event.
	// No pointers to scripts are kept, in case scripts are destroyed.
	struct WaitingScriptEntry
	{
		// the unique ID of the script
		uint32					mScriptId;
		#ifdef __NOPT_ASSERT__
		uint32					mScriptName;
		#endif
		// 0 denotes a dead entry
		uint32				  	mEventType;
		// the object on which the script is running
		uint32					mObjectId;
	};

	enum
	{
		vMAX_SCRIPT_ENTRIES 	= 40,
		vDEAD_SCRIPT_ENTRY 		= 0,
	};

	WaitingScriptEntry			m_waiting_script_tab[vMAX_SCRIPT_ENTRIES];

	bool						m_debug;
};
	


bool ScriptLaunchEvent(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptWaitForEvent(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptObjectExists(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptTerminateObjectsScripts(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAssignAlias(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetObjectProperties(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPrintEventLog(Script::CStruct *pParams, Script::CScript *pScript);



} // end namespace

#endif

