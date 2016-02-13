/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Object (OBJ)											**
**																			**
**	File name:		gel/object.h											**
**																			**
**	Created: 		05/27/99	-	mjb										**
**																			**
**  Notes:
**
**  (Mick) This is the base type for all objects in the game.  It contains basic
**  mechanisms for maintaining a list of object associated with Object Servers
**  and procedures for deleting them
**  There are also hooks into the CScript class, in that an object has a 
**  reference to a script, and a virtual function CallMember, which allows 
**  the script to call a member function by name (actually by checksum of the name)  
**
**
*****************************************************************************/

#ifndef __GEL_OBJECT_H
#define __GEL_OBJECT_H

#define	__SCRIPT_EVENT_TABLE__		// define this is you want event tables attached to scripts, rather than objects

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/list.h>
#include <core/macros.h>
#include <core/math.h>

#include <sys/timer.h>

#include <gel/ObjPtr.h>
#include <gel/RefCounted.h>

namespace Script
{
	class CStruct;
	class CScript;
}	

struct SBBox{
	Mth::Vector m_max;
	Mth::Vector m_min;
	Mth::Vector centerOfGravity;
	float m_radius;  // will give the max distance squared where there may still be a collision.
};

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define OBJECT_FLAG_INVISIBLE	1

namespace Front
{
	class CScreenElementManager;
}

namespace Script
{
	class CArray;
}

enum EReplaceEventHandlers
{
	DONT_REPLACE_HANDLERS = 0,
	REPLACE_HANDLER
};


namespace Obj			 
{

/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

class CEvent;
class CEventHandlerTable;
class CBaseManager;
class CGeneralManager;
class CObject;
typedef CSmtPtr<CObject> CObjectPtr;


/*
	CObject is described in more detail in: Q:\sk4\Docs\programming\Object Scripting System.htm
	
	The base class for all scriptable objects. Every CObject has a global ID that
	is unique among all CObjects. It also has a type specifier.
	
	A script that is running can reference a CObject. The script can access special
	commands for that CObject through CallMemberFunction().	Tags, which are like
	scriptable member variables, can be attached from CObjects from script, too.
	
	Event handling is supported through the PassTargetedEvent() function.
*/
class  CObject  : public CRefCounted
{
  	
	friend class		CBaseManager;
	friend class		CGeneralManager;
	friend class		Front::CScreenElementManager;

public :						
						CObject();
	virtual				~CObject ( void );

	void				DestroyIfUnlocked ();				// Destroy immediately 

	void				AddReference ( void );
	void				RemoveReference ( void );
	bool				IsReferenced() {return (m_ref_count > 0);}

	bool				IsDead ( void ) const;				// Access functions for the above
	bool				IsLocked ( void ) const;
		
	uint32				GetID() const  {return m_id;}
	sint				GetType() const {return m_type;}
	void				SetID(uint32 id);
	void				SetType(sint type);
	
	void				ClearStampBit(uint32 stamp_mask) { m_stamp &= ~stamp_mask; }
	bool				CheckStampBit(uint32 stamp_mask) const { return m_stamp & stamp_mask; }
	void				SetStampBit(uint32 stamp_mask) { m_stamp |= stamp_mask; }

	uint32				GetFlags() const {return m_object_flags;}
	void				SetFlags(uint32 flags) {m_object_flags = flags;}
	void				SetLockOn() {m_object_flags |= vLOCKED;}
	void				SetLockOff() {m_object_flags &= ~vLOCKED;}
	
	#ifdef	__NOPT_ASSERT__		
	void				SetLockAssertOn() {m_object_flags |= vLOCKEDASSERT;}
	void				SetLockAssertOff() {m_object_flags &= ~vLOCKEDASSERT;}
	bool				GetLockAssert() {return m_object_flags & vLOCKEDASSERT;}
	#endif

	Script::CScript	*	GetScript() const {return mp_script;}
	void				SetScript(Script::CScript* p_script) {mp_script = p_script;}



	void  				SetEventHandlers(Script::CArray *pArray, EReplaceEventHandlers replace = DONT_REPLACE_HANDLERS);
	void				RemoveEventHandler(uint32 type);
	void				RemoveEventHandlerGroup(uint32 group);
	
	// See details on tags in object document
	void				SetIntegerTag(uint32 name, int value);
	void				SetChecksumTag(uint32 name, uint32 value);
	void				RemoveFlagTag(uint32 name);
	bool				GetIntegerTag(uint32 name, int *pResult) const ;
	bool				GetChecksumTag(uint32 name, uint32 *pResult) const;
	bool				ContainsFlagTag(uint32 name) const;
	void				SetTagsFromScript(Script::CStruct *pStruct);
	void 				RemoveTagsFromScript(Script::CArray *pNameArray);
	void				CopyTagsToScriptStruct(Script::CStruct *pStruct);
	void 				SetVectorTag(uint32 name, Mth::Vector	v);
	bool 				GetVectorTag(uint32 name, Mth::Vector *pResult) const;


	void 				SetOnExceptionScriptChecksum(uint32 OnExceptionScriptChecksum);
	uint32 				GetOnExceptionScriptChecksum() const; 

	
	// subclasses that extend next three functions should still call CObject versions
	virtual void		SetProperties(Script::CStruct *pProps);
	virtual bool 		CallMemberFunction (uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript);	// Call a member function based on the checksum of the function name
	virtual void		GetDebugInfo(Script::CStruct *p_info);
	
	// Called by CTracker singleton to pass events to this CObject
	virtual bool		PassTargetedEvent(CEvent *pEvent, bool broadcast = false);	// return true if object still valid

	virtual void		HideForReplayPlayback() {}
	virtual void		RestoreAfterReplayPlayback() {}
	
												  
	enum
	{
		vDEAD						= ( 1 << 0 ),
		// if locked, can't be deleted
		vLOCKED						= ( 1 << 1 ),
		vINVISIBLE					= ( 1 << 2 ),

		// this flag has been moved to the ExceptionComponent:
		//		vDISABLE_EXCEPTIONS			= ( 1 << 3 ),
	
	#ifdef	__NOPT_ASSERT__	
		vLOCKEDASSERT				= ( 1 << 4 ),
	#endif
		vCOMPOSITE				= ( 1 << 5 ),
		
		
		// RJM: flags from 8 on are reserved for children of CObject
		
	};

protected:

						CObject ( CBaseManager* );
	
	void				allocate_tags_if_needed();

public:	
	void 				AllocateScriptIfNeeded();
	
	void 				debug_validate_smart_pointers(CSmtPtr<CObject> *pPtrToCheckForInclusion = NULL);
	void				MarkAsDead( void );			// Safe kill called from within the object's logic

	CBaseManager *			GetManager() const	{return mp_manager;}	// just for exception component stuff

protected:
	
	uint32				m_object_flags;
	
	CBaseManager *			mp_manager;
	
	// object script stuff:
	Script::CScript*	mp_script;
	
	// object ID, globally unique among CObjects
	uint32				m_id;
	sint				m_type;						// Object type game dependent
	uint32				m_stamp;

#ifndef	__SCRIPT_EVENT_TABLE__		
	CEventHandlerTable *	mp_event_handler_table;
	
	// Script to run when there is an exception	
	// (needed here, since we are getting rid of CExceptionComponent, to replace it with one 
	// single event system
	uint32 			mOnExceptionScriptChecksum;
#endif	 

	// Tags are basically variables that can be attached and queried by the scripting
	// system at runtime. Very convenient for providing state info.
	Script::CStruct	*		mp_tags;	 

public:
	// (Moved here from CMovingObject)	
	// A set of flags that can be set using the SendFlag script command. Used for sending messages to objects.
	uint32  mScriptFlags;
	uint32	GetFlags( Script::CStruct *pParams, Script::CScript *pScript ) const;

	// K: Used by CScript::TransmitInfoToDebugger to determine whether the CScript is the
	// main script of it's parent object.
	bool	MainScriptIs(const Script::CScript *p_script) const {return mp_script==p_script;}
	
	// standard objects can not be paused; thus, spawned (non-main) scripts running on them should never be paused
	virtual bool		ShouldUpdatePauseWithObjectScripts (   ) { return true; }
	
	// K: Also used by CScript::TransmitInfoToDebugger
	Script::CStruct *GetTags() const {return mp_tags;}

public:
	// GJ:  Made this public so that components can set their
	// parent objects' scripts
	void				SwitchScript( uint32 scriptChecksum, Script::CStruct *pParams );
	Script::CScript*	SpawnScriptPlease( uint32 scriptChecksum, Script::CStruct *pParams, int Id = 0, bool pause_with_object = false );
	void				SpawnAndRunScript( uint32 ScriptChecksum, int node = -1, bool net_script = false, bool permanent = false, Script::CStruct *p_params = NULL );
	void				SpawnAndRunScript( const char *pScriptName, int node = -1, bool net_script = false, bool permanent = false, Script::CStruct *p_params = NULL );

	void				CallScript( uint32 ScriptChecksum, Script::CStruct *pParams );

	void 				SelfEvent(uint32 event, Script::CStruct *pData = NULL);
	void				BroadcastEvent(uint32 event, Script::CStruct *pData = NULL, float radius = 0.0f);
	
protected:
	Lst::Node< CObject >	m_node;
	
private:
	int						m_ref_count;
};




/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

void		DestroyIfUnlocked ( CObject* obj, void* data = NULL );
void		SetLockOff ( CObject* obj, void* data = NULL );
CObject *	ResolveToObject(uint32 id);

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

inline	void			CObject::AddReference ( void )
{
	m_ref_count++;
}


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void			CObject::RemoveReference ( void )
{
	//Dbg_Assert(m_ref_count > 0);
	if (m_ref_count > 0)
	{
		m_ref_count--;   	
	}
	// XXX
	else
	{
		#ifdef	__NOPT_ASSERT__
		printf("Aaaah!! Removing more references than there are\n");
		#endif
		Dbg_Assert(( m_object_flags & ( 1 << 16 )) == 0 );
//		if (m_object_flags & (1<<16)) //Front::CScreenElement::vIS_SCREEN_ELEMENT
//			Dbg_Assert(0);
	
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	bool			CObject::IsLocked ( void ) const 
{
	return ( m_object_flags & ( vLOCKED ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	bool			CObject::IsDead ( void ) const 
{
   	
	return ( m_object_flags & ( vDEAD ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj

#endif	// __GEL_OBJECT_H
