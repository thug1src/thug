#ifndef __GEL_EVENT_H
#define __GEL_EVENT_H

#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif

namespace Script
{
	class CScript;
	class CStruct;
	class CArray;
}


namespace Obj
{

class CObject;
class CTracker;


class CEvent
{
	friend class CTracker;

public:

	enum EEventLevel
	{
		// applies to target or source
		vSYSTEM_EVENT =	0x36b2ee74		// "system"
	};

	// should also add new event types to EventLog.q
	enum EEventType
	{
		// applies to type
		TYPE_FOCUS					= 0x9d3fb516,
		TYPE_UNFOCUS				= 0x4adf0cd3,
		TYPE_NOTIFY_CHILD_LOCK		= 0x3056434f,
		TYPE_NOTIFY_CHILD_UNLOCK	= 0x4a57316b,
		TYPE_PAD_UP					= 0x30a4f836,
		TYPE_PAD_DOWN  				= 0x0fd1ac26,
		TYPE_PAD_LEFT  				= 0x6949db75,
		TYPE_PAD_RIGHT 				= 0x05ddd87c,
		TYPE_PAD_CHOOSE				= 0x1e3e253b,
		TYPE_PAD_BACK				= 0x7ee0fd2a,
		TYPE_PAD_SQUARE	   			= 0x5c3979e3,
		TYPE_PAD_CIRCLE	   			= 0x456d7433,
		TYPE_PAD_L1					= 0xaa7f2028,
		TYPE_PAD_L2					= 0x33767192,
		TYPE_PAD_L3					= 0x44714104,
		TYPE_PAD_R1					= 0x7e3e1ff7,
		TYPE_PAD_R2					= 0xe7374e4d,
		TYPE_PAD_R3					= 0x90307edb,
		TYPE_PAD_START 				= 0x2e6ef8e7,
		TYPE_PAD_SELECT				= 0xda28fb8a,
		
		// these have been deprecated
		// Ryan: now they are reprecated again!
		TYPE_PAD_X					= 0x6fb6a322,
		TYPE_PAD_TRIANGLE			= 0xfe6869df,
	};

	enum EFlags
	{
		mREAD						= (1<<0),
		mHANDLED					= (1<<1),
		mCONTROLS_OWN_DATA			= (1<<2),
	};

	uint32						GetType() {return m_type;}
	uint32						GetSource() {return m_source_id;}
	uint32						GetTarget() {return m_target_id;}
	Script::CStruct *			GetData() {return mp_data;}

	void						MarkRead(uint32 receiverId = 0, uint32 script = 0);
	void						MarkHandled(uint32 receiverId = 0, uint32 script = 0);
	bool						WasHandled() {return (m_flags & mHANDLED);}

	/*
		Helper functions 
	*/

	// for focus, pad events
	static int					sExtractControllerIndex(CEvent *pEvent);

protected:

								CEvent();
								~CEvent();
	
	uint32						m_type;
	uint32						m_source_id;
	uint32						m_target_id;
	uint32						m_flags;

	Script::CStruct *			mp_data;
};




/*
	Notes:
	-the event_filter() function automatically locks the associated CObject, so that no 
	event handling code or script will delete it during event_filter() or pass_event_to_listener(). 
	The danger is the CObject will delete the CEventListener.
*/
class CEventListener
{
	friend class CTracker;
	
public:

								CEventListener();
	virtual						~CEventListener();
	void						RegisterWithTracker(CObject *pObject);

protected:

	void						event_filter(CEvent *pEvent);
	
	// implement this function in subclasses for class-specific event handling
	virtual void				pass_event_to_listener(CEvent *pEvent) = 0;

	
	CObjectPtr					mp_object;
	CEventListener *			mp_next_listener;
	bool						m_registered;
	int							m_ref_count;
};




class CEventHandlerTable
{
	friend class Script::CScript;
	friend class CObject;
	friend class CTracker;

public:
	enum 
	{
		// when used, gets assigned to 'script' member of entry
		vDEAD_ENTRY				= CRCC(0xc377c572, "null_script")
	};
	
	enum
	{
		vDEFAULT_GROUP			= CRCC(0x1ca1ff20, "Default")
	};
	
	
	void	GetDebugInfo(Script::CStruct *p_info);

public:  // temp public, for skater cleanup to prevent fragmentation
#ifdef	__SCRIPT_EVENT_TABLE__			
	void						unregister_all(Script::CScript *p_script);
#else
	void						unregister_all(CObject *p_object);
#endif

	void						AddEvent(uint32 ex, uint32 scr, uint32 group, bool exception, Script::CStruct *p_params);
	
	static void					sPrintTable ( CEventHandlerTable* table );
protected:

								CEventHandlerTable();
								~CEventHandlerTable();


	void						add_from_script(Script::CArray *pArray, bool replace = false);
	void						remove_entry(uint32 type);
	void						remove_group(uint32 group = vDEFAULT_GROUP);
	void						compress_table();
	void						set_event_enable(uint32 type, bool state);
	
#ifdef	__SCRIPT_EVENT_TABLE__		
	void						pass_event(CEvent *pEvent, Script::CScript *pScript, bool broadcast = false);
	void						register_all(Script::CScript *p_script);
#else	
	void						pass_event(CEvent *pEvent, CObject *pObject, bool broadcast = false);
	void						register_all(CObject *p_object);
#endif	
	
	struct Entry
	{
		uint32					type;
		uint32					script;
		uint32					group;
		
		// if false, script won't be run when event
		// of matching type received
		bool					enabled;
		
		// if "exception" is true, then the script is jumped to, rather than called.
		bool					exception;
		
		// if "broadcast" is true, then this handler will also respont to broadcast events
		bool					broadcast;
			
		Script::CStruct		*	p_params;
	};

#ifdef	__NOPT_ASSERT__
public:
#endif
	Entry *						mp_tab;
	int							m_num_entries;
	bool						m_valid;
	bool						m_changed;		// set if table changed while running an event
	int							m_in_immediate_use_counter; // by pass_event()
};



}
#endif
