/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Objects (OBJ) 											**
**																			**
**	File name:		object.cpp												**
**																			**
**	Created:		05/27/99	-	mjb										**
**																			**
**	Description:	Game object code										**
**																			**
*****************************************************************************/

// start autoduck documentation
// @DOC object.cpp
// @module object | None
// @subindex Scripting Database
// @index script | object

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gel/scripting/struct.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/component.h>
#include <gel/object.h>
#include <gel/ObjPtr.h>
#include <gel/objman.h>
#include <gel/objtrack.h>
#include <gel/objsearch.h>
#include <gel/Event.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/



namespace Script
{
	extern void PrintContents(CStruct *p_structure, int indent);
}

namespace Obj
{



/*****************************************************************************
**								Private Types								**
*****************************************************************************/

// temp
// temp	  REMOVE THIS WHEN script.h is implemented
// temp
//class	CScript
//{
//};
// temp
// temp
// temp


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/*
	Creates CObject, does not give it an ID or register it.
*/
CObject::CObject() :
	m_node(this)
{
	m_id = CBaseManager::vNO_OBJECT_ID;	
	m_type = CBaseManager::vNO_OBJECT_TYPE;	
#ifndef	__SCRIPT_EVENT_TABLE__		
	mp_event_handler_table = NULL;
#endif	
	mp_tags = NULL;
	mp_script = NULL;
	mp_manager = NULL;
	m_object_flags = 0;
	
	m_ref_count = 0;
	
	m_stamp = 0;
}




/*
	Creates CObject, registers it with manager, giving it automatic ID.
*/
CObject::CObject( CBaseManager* p_obj_manager )
	: m_node(this)
//: manager( obj_manager )

{
	Dbg_Assert(p_obj_manager);	

	m_id = CBaseManager::vNO_OBJECT_ID;	
	m_type = CBaseManager::vNO_OBJECT_TYPE;	
	p_obj_manager->RegisterObject( *this );	// add to manager's object list
#ifndef	__SCRIPT_EVENT_TABLE__		
	mp_event_handler_table = NULL;
#endif	
	mp_tags = NULL;
	mp_script = NULL;
	mp_manager = NULL;
	m_object_flags = 0;
	
	m_ref_count = 0;


}




/*
	Destroying CObject disconnects it from manager, stops all scripts running on it.
*/
CObject::~CObject()
{
	
	#ifdef	__NOPT_ASSERT__	
	if (GetLockAssert())
	{
		if (mp_script)
		{	
			Dbg_MsgAssert(0,("\n%s\n Object %s at %p has been deleted while updating a script running on it, maybe goal manager callback? See Mick.",mp_script->GetScriptInfo(),Script::FindChecksumName(GetID()),this));
		}
		Dbg_MsgAssert(0,("Object %s at %p has been deleted while updating a script running on it, maybe goal manager callback? See Mick.",Script::FindChecksumName(GetID()),this));
	}
	#endif
	
	Dbg_MsgAssert(!IsLocked(), ("can't destroy CObject, is locked"));
	
	if (mp_tags)
		delete mp_tags;

#ifndef	__SCRIPT_EVENT_TABLE__		
	if (mp_event_handler_table)
	{
		// Remove the references to the event handler from the tracker
		mp_event_handler_table->unregister_all(this);
		
		if (mp_event_handler_table->m_in_immediate_use_counter)
		{
			// table still in use by its pass_event() function,
			// so leave it up to that function to destroy the table
			mp_event_handler_table->m_valid = false;
		}
		else
		{
			delete mp_event_handler_table;
		}
	}
#endif

	if (mp_manager)
	{
		mp_manager->UnregisterObject( *this );
	}

	if (mp_script)
	{
		delete mp_script;
		mp_script = NULL;
	}
	
	// Stop any scripts that have this object as their parent.
	// Note: This won't actually delete the scripts, to prevent dangling pointers.
	Script::StopAllScriptsUsingThisObject(this);
}




/*
	Used by object managers when they delete all but a few objects (which have been locked).
*/
void CObject::DestroyIfUnlocked()
{	   
	if ( !IsLocked() )
	{
		delete this;
	}
}




/*
	Sets the global ID of this CObject. A CObject needs an ID in order to function
	in the system. If no ID is assigned, it will automatically be assigned one 
	when registered with a manager.
	
	Calling this function on a CObject that already has an ID will attempt to change
	its ID globally (whereever that ID is used).
*/
void CObject::SetID(uint32 id) 
{	
	if (m_id != CBaseManager::vNO_OBJECT_ID && mp_manager)
	{
		mp_manager->ReregisterObject(*this, id);
	}
	m_id = id;
	SetChecksumTag(CRCD(0x40c698af, "id"), m_id);
}




/*
	Type is pretty much just the checksum of the class, minus the "C"
*/
void CObject::SetType(sint type) 
{
	Dbg_MsgAssert(m_type == CBaseManager::vNO_OBJECT_TYPE, ("CObject already assigned a type"));
	m_type = type;
	SetChecksumTag(CRCD(0x7321a8d6, "type"), m_type);
}


#ifndef	__SCRIPT_EVENT_TABLE__		

/*
	Sets up the table that specifies which scripts to run in response to which events.
	
	See object scripting document.
*/
void CObject::SetEventHandlers(Script::CArray *pArray, EReplaceEventHandlers replace)
{
	
	if (!mp_event_handler_table)
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		mp_event_handler_table = new CEventHandlerTable();
		Mem::Manager::sHandle().PopContext();
	}
	else
	{
		mp_event_handler_table->unregister_all(this);	
	}
	mp_event_handler_table->add_from_script(pArray, replace);
//	mp_event_handler_table->compress_table();
	
	mp_event_handler_table->register_all(this);

}



/*
	Removes an entry in the event table with the given type id
*/
void CObject::RemoveEventHandler(uint32 type)
{
	if (!mp_event_handler_table) return;
	mp_event_handler_table->remove_entry(type);

// Mick:  by not compressing the event handler table after removing entries
// we should minimize the amount of allocs needed
// new event handlers can just re-use these "removed" sloes
//	mp_event_handler_table->compress_table();

// Refresh the Obj::Tracker

	Obj::CTracker::Instance()->UnregisterEventReceiver(type, this); 
		
}



/*
	Removes all entries in the event table with the given group id
*/
void CObject::RemoveEventHandlerGroup(uint32 group)
{
	if (!mp_event_handler_table) return;
	
	mp_event_handler_table->unregister_all(this);	
	
	mp_event_handler_table->remove_group(group);

// Mick:  by not compressing the event handler table after removing entries
// we should minimize the amount of allocs needed
// new event handlers can just re-use these "removed" sloes
//	mp_event_handler_table->compress_table();
	
	mp_event_handler_table->register_all(this);	

}

#else


/*
	Sets up the table that specifies which scripts to run in response to which events.
	
	See object scripting document.
*/
void CObject::SetEventHandlers(Script::CArray *pArray, EReplaceEventHandlers replace)
{
	// TEMP - Pass into script object.  Eventually handled soley by script with no need for object
	AllocateScriptIfNeeded();
	mp_script->SetEventHandlers(pArray, replace);
}

/*
	Removes an entry in the event table with the given type id
*/
void CObject::RemoveEventHandler(uint32 type)
{
	if (!mp_script)
	{
		return;
	}
	mp_script->RemoveEventHandler(type );
}

/*
	Removes all entries in the event table with the given group id
*/
void CObject::RemoveEventHandlerGroup(uint32 group)
{
	if (!mp_script)
	{
		return;
	}
	mp_script->RemoveEventHandlerGroup(group);
}



#endif


void CObject::SetIntegerTag(uint32 name, int value)
{
	allocate_tags_if_needed();

	mp_tags->AddInteger(name, value);
}




void CObject::SetChecksumTag(uint32 name, uint32 value)
{
	allocate_tags_if_needed();

	mp_tags->AddChecksum(name, value);
}




void CObject::RemoveFlagTag(uint32 name)
{
	if (mp_tags)
		mp_tags->RemoveFlag(name);
}




bool CObject::GetIntegerTag(uint32 name, int *pResult) const
{
	if (mp_tags)
		return mp_tags->GetInteger(name, pResult);
	return false;
}




bool CObject::GetChecksumTag(uint32 name, uint32 *pResult) const 
{
	if (mp_tags)
		return mp_tags->GetChecksum(name, pResult);
	return false;
}




bool CObject::ContainsFlagTag(uint32 name) const
{
	if (mp_tags)
		return mp_tags->ContainsFlag(name);
	return false;
}


// Add a vector to the tag structure with name "name", and copy in the vector "v"
void CObject::SetVectorTag(uint32 name, Mth::Vector	v)
{
	allocate_tags_if_needed();

	mp_tags->AddVector(name, v[X], v[Y], v[Z]);
}


// get the value a vector tag.
// returning false if not found
bool CObject::GetVectorTag(uint32 name, Mth::Vector *pResult) const
{
	if (mp_tags)
		return mp_tags->GetVector(name, pResult);
	return false;
}

#ifndef	__SCRIPT_EVENT_TABLE__		

void 	CObject::SetOnExceptionScriptChecksum(uint32 OnExceptionScriptChecksum) 
{
	mOnExceptionScriptChecksum = OnExceptionScriptChecksum;
}

uint32 	CObject::GetOnExceptionScriptChecksum() const 
{
	return mOnExceptionScriptChecksum;
}
#else
// Todo - Pass these to the script

#endif


void CObject::SetTagsFromScript(Script::CStruct *pStruct)
{
	allocate_tags_if_needed();

	mp_tags->AppendStructure(pStruct);
	//printf("Set tags of object %s to:\n", Script::FindChecksumName(m_id));
	//Script::PrintContents(mp_tags, 4);
}




void CObject::RemoveTagsFromScript(Script::CArray *pNameArray)
{
	if (!mp_tags) return;

	for (uint i = 0; i < pNameArray->GetSize(); i++)
	{
		uint32 name_crc = pNameArray->GetChecksum(i);
		mp_tags->RemoveComponent(name_crc);
		mp_tags->RemoveFlag(name_crc);
	}
	
	if (mp_tags->IsEmpty())
	{
		delete mp_tags;
		mp_tags = NULL;
	}
}




void CObject::CopyTagsToScriptStruct(Script::CStruct *pStruct)
{
	Dbg_Assert(pStruct);
	
	if (mp_tags)	
	{
		pStruct->AppendStructure(mp_tags);
		//printf("Fetching tags of object %s:\n", Script::FindChecksumName(m_id));
		//Script::PrintContents(pStruct, 4);
	}
}




/*
	Property setting script commands enter through this function.
*/
void CObject::SetProperties(Script::CStruct *pProps)
{
	bool replace_handlers = pProps->ContainsFlag("replace_handlers");

	Script::CArray *p_event_handler_table;
	if (pProps->GetArray("event_handlers", &p_event_handler_table))
	{
#ifdef	__SCRIPT_EVENT_TABLE__		
		CObject::SetEventHandlers(p_event_handler_table, EReplaceEventHandlers(replace_handlers));
#else	
		CObject::SetEventHandlers(p_event_handler_table, EReplaceEventHandlers(replace_handlers));
#endif	
	}
	/*
	else if (pProps->GetArray("event_handler", &p_event_handler_table))
	{
		// just a single entry (makes for easier-to-read script)
		CObject::SetEventHandlers(p_event_handler_table, replace_handlers, true);
	}
	*/

	Script::CStruct *p_tags;
	if (pProps->GetStructure("tags", &p_tags)) 
		SetTagsFromScript(p_tags);

	Script::CArray *p_remove_tags;
	if (pProps->GetArray("remove_tags", &p_remove_tags))
		RemoveTagsFromScript(p_remove_tags);
}




/******************************************************************/
/*   CObject::CallMemberFunction                                  */
/*   Call a member function, based on a checksum                  */
/*   This is usually the checksum of the name of the function     */
/*   but can actually be any number, as it just uses a switch     */
/*   note this is a virtual function, so the same checksum        */
/*   can do differnet things for different objects                */
/*                                                                */
/*                                                                */
/******************************************************************/

bool CObject::CallMemberFunction( uint32 Checksum, Script::CScriptStructure *pParams, Script::CScript *pScript )
{
    
	Dbg_MsgAssert(pScript,("NULL pScript"));

	switch(Checksum)
	{
		case CRCC(0x3611c136, "GetTags"):
			CopyTagsToScriptStruct(pScript->GetParams());
			break;

		case CRCC( 0xa58079eb, "SetTags" ):
			SetTagsFromScript( pParams );
			break;
			 
		case 0xc6870028:// "Die"
			MarkAsDead( );
			break;	
		        
		case 0xb3c262ec:// "DisassociateFromObject"
			pScript->DisassociateWithObject(this);
			break;	
		        
		// @script | Obj_GetId | 
		case 0x500eb224: // Obj_GetId
			Dbg_Assert( pScript && pScript->GetParams() );
			pScript->GetParams()->AddChecksum( "objId", m_id ); 
		break;
		
		// @script | Obj_FlagSet | Check to see if a flag has been set.  Flags can 
		// be defined anywhere, but you should keep them in your personal scripts file.  
		// It is important that you prefix all flags with your initials to ensure that 
		// there are no conflicts with the other designers.
		// <nl>Example 1:
		// <nl>MJD_LAMP_GOT_BROKEN = 14
		// <nl>MJD_LAMP_GOT_HIT = 15
		// <nl>MJD_LAMP_GOT_DISABLED = 16
		// <nl>if Obj_FlagSet 15
		// <nl>    Obj_ClearFlag MJD_LAMP_GOT_HIT
		// <nl>    WiggleProfusely
		// <nl>endif
		// <nl>Example 2:
		// <nl>if Obj_FlagSet MJD_LAMP_GOT_HIT clear
		// <nl>    WiggleProfusely
		// <nl>endif
		// <nl>Example 3:
		// <nl>If Obj_FlagSet JKU_FLAG_PED_START  // JKU_FLAG_PED_START would be defined in JKU_Scripts.q
		// <nl>     Printf “Yes”
		// <nl>endif
		// @flag all | Clear all flags
		// @flag clear | Clear the flag
		// @flag reset | I just added a 'reset' flag you can send in to clear 
		// the flag after checking it, if it's set
		case 0x4babc987: // Obj_FlagSet
		{
			uint32 scriptFlags = mScriptFlags;
			uint32 Flags = 0;
			Flags = GetFlags( pParams, pScript );
			if ( !Flags )
			{
				Dbg_MsgAssert( 0,( "\n%s\nObj_FlagSet command requires a flag to be specified.\n(Either an integer or a name defined to be an integer)",pScript->GetScriptInfo()));
				return ( false );
			}
			if ( pParams->ContainsFlag( 0x1a4e0ef9 ) ) // clear
			{
				mScriptFlags &= ~( Flags );
			}
			if ( scriptFlags & Flags )
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		}		
		
		// @script | Obj_FlagNotSet | Check to see if a flag has not been set.
		case 0x53ebee03: // Obj_FlagNotSet
		{
			uint32 Flags = 0;
			Flags = GetFlags( pParams, pScript );
			if ( !Flags )
			{
				Dbg_MsgAssert( 0,( "\n%s\nObj_FlagNotSet command requires a flag to be specified.\n(Either an integer or a name defined to be an integer)",pScript->GetScriptInfo()));
				return ( false );
			}
			if ( mScriptFlags & ( Flags ) )
			{
				return false;
			}
			else
			{
				return true;
			}
			break;
		}		
		
		// @script | Obj_ClearFlag | Object member function which clears the specified flag or flags.
		// <nl>There are 3 ways of using it, it can clear just one flag, or an array of flags, 
		// or all the flags.
		// <nl>Example 1:
		// <nl>Obj_ClearFlag JKU_FLAG_PED_START  // Set the flag to 0
		// <nl>Example 2:
		// <nl>Obj_ClearFlag [ JKU_FLAG_PED_START JKU_FLAG_PED_STOP ]  // Clears an array of flags
		// <nl>Example 3:
		// <nl>Obj_ClearFlag All  // Clears all the flags.
		// @flag all | Clear all the flags
		case 0x6c2b67f9: // Obj_ClearFlag
		{
			if (pParams->ContainsFlag(0xc4e78e22/*All*/))
			{
				mScriptFlags=0;
				return true;
			}	
			uint32 Flags = 0;
	
			Flags = GetFlags( pParams, pScript );
			if ( !Flags )
			{
				Dbg_MsgAssert( 0,( "\n%s\nObj_ClearFlag command requires a flag to be specified.\n(Either an integer or a name defined to be an integer)",pScript->GetScriptInfo()));
				return ( false );
			}
			mScriptFlags &= ~Flags;
			break;
		}
	
		// @script | Obj_SetFlag | Sets the flag (or an array of flags) on an object.
		// <nl>Same parameters as Obj_ClearFlag (except doesn't recognize the 
		// 'all' parameter).
		case 0xbe563426: // Obj_SetFlag
		{
			uint32 Flags = 0;
			Flags = GetFlags( pParams, pScript );
			if ( !Flags )
			{
				Dbg_MsgAssert( 0,( "\n%s\nObj_SetFlag command requires a flag to be specified.\n(Either an integer or a name defined to be an integer)",pScript->GetScriptInfo()));
				return ( false );
			}
			mScriptFlags |= Flags;
			break;
		}

		// @script | Obj_KillSpawnedScript | Kills a spawned script.  Can be passeed a name or id.
		// @parm name | name | Name of the script you want to spawn (no quotes)
		// @parm name | id | id of the script you want to spawn (no quotes)
		case 0xfbd89cd5: // Obj_KillSpawnedScript
		{
			uint32 ScriptChecksum=0;
			if (pParams->GetChecksum(CRCD(0xa1dc81f9,"Name"),&ScriptChecksum))
			{
				// Got a script name, so kill all spawned scripts that ran that script.
				// BUT NOT THE SCRIPT CALLING THIS!
				Script::KillSpawnedScriptsWithObjectAndName( this, ScriptChecksum, pScript );
				return true;
			}
		
			uint32 Id=0;										   
			if (pParams->GetChecksum(CRCD(0x40c698af,"Id"),&Id))
			{
				// They specified an Id, so kill all spawned scripts with this Id,
				// BUT NOT THE SCRIPT CALLING THIS!
				Script::KillSpawnedScriptsWithObjectAndId( this, Id, pScript );
				return ( true );
			}
			Dbg_MsgAssert( 0,( "\n%s\nMust specify Name or ID on Obj_KillSpawnedScript", pScript->GetScriptInfo() ));
			return true;
			break;
		}
	
		// @script | Obj_SpawnScript | Causes the object to run a script, in parallel
		// to whatever script is running on the object.
		// @flag ScriptName | Name of the script you want to spawn (no quotes)
		// @parmopt name | Params | {} | Any parameters you want to pass to the script being
		// spawned.  Must surround params in { }
		case 0x23a4e5c2: // Obj_SpawnScript
		{
			Script::CComponent* p_component = pParams->GetNextComponent();
			if ( p_component && p_component->mType == ESYMBOLTYPE_NAME )
			{
				uint32 scriptChecksum = p_component->mChecksum;
				
				// The spawned script can optionally be given an Id, so that it can be deleted
				// by KillSpawnedScript.
				uint32 Id=0;
				// keep the same ID as the parent if not specified...
				Id = Script::FindSpawnedScriptID(pScript);
				pParams->GetChecksum("Id",&Id);
				Script::CScriptStructure *pScriptParams = NULL;
				pParams->GetStructure( "Params", &pScriptParams );
				#ifdef __NOPT_ASSERT__	
				Script::CScript *p_script=SpawnScriptPlease( scriptChecksum, pScriptParams, Id, pParams->ContainsFlag(CRCD(0x8757d0bb, "PauseWithObject")) );
				p_script->SetCommentString("Created by Obj_SpawnScript");
				p_script->SetOriginatingScriptInfo(pScript->GetCurrentLineNumber(),pScript->mScriptChecksum);
				#else
				SpawnScriptPlease( scriptChecksum, pScriptParams, Id );
				#endif

			}
			break;
		}
	
		// @script | Obj_SwitchScript | Causes the object to replace the current script 
		// attached to it with the script specified by ScriptName. <nl>
		// Can use the pass params just like Obj_SpawnScript.
		// @flag ScriptName | Name of the script you want to spawn (no quotes)
		case 0x714937c7: // Obj_SwitchScript
		{
			uint32 scriptChecksum;
			if ( pParams->GetChecksum( NONAME, &scriptChecksum ) )
			{
				Script::CScriptStructure *pScriptParams = NULL;
				pParams->GetStructure( "Params", &pScriptParams );
				SwitchScript( scriptChecksum, pScriptParams );
			}
			break;
		}
		
		
		// case	CRCC(0x6df6caef,"SetProperties"):
		case	CRCC(0x6c63c7c5,"SetProps"):
		{
			// Lowest level SetProperties, allowing ANY object to set event handlers
			SetProperties(pParams);
			break;
		}
		
/*		
		case	CRCC(0x1127430c, "ClearEventHandler"):
		{
			uint32 type;
			pParams->GetChecksum(NO_NAME, &type, Script::ASSERT);
			RemoveEventHandler(type);
			break;
		}
			
			
		case	CRCC(0x8968da7f, "ClearEventHandlerGroup"):
		{
			uint32 group = CEventHandlerTable::vDEFAULT_GROUP;
			pParams->GetChecksum(NO_NAME, &group);
			RemoveEventHandlerGroup(group);
			break;
		}
		
// @script | OnExceptionRun | run the specified script on exception
// @uparm name | script name to run
// can be called without a parameter to clear it
//		case 0x2c0c9e7b: // OnExceptionRun	
		case CRCC(0x2c0c9e7b,"OnExceptionRun"):
		{
			uint32 OnExceptionScriptChecksum = 0;
			pParams->GetChecksum( NONAME, &OnExceptionScriptChecksum);
			#ifdef	__SCRIPT_EVENT_TABLE__		
			if (mp_script)
			{
				mp_script->SetOnExceptionScriptChecksum(OnExceptionScriptChecksum);
			}
			#else
			SetOnExceptionScriptChecksum(OnExceptionScriptChecksum);
			#endif
			break;
		}

		case CRCC(0xbefaa466,"OnExitRun"):
		{
			uint32 OnExitScriptChecksum = 0;
			pParams->GetChecksum( NONAME, &OnExitScriptChecksum);
			#ifdef	__SCRIPT_EVENT_TABLE__		
			if (mp_script)
			{
				mp_script->SetOnExitScriptChecksum(OnExitScriptChecksum);
			}
			#else
			SetOnExitScriptChecksum(OnExitScriptChecksum);
			#endif
			break;
		}

*/

		case 0x20d125d7: // Obj_Visible
			m_object_flags &= ~vINVISIBLE;
			break;

		case 0x3578e5a9: // Obj_Invisible
			m_object_flags |= vINVISIBLE;
			break;


		
		default:
			Dbg_MsgAssert(0,("\n%s\nNo CObject member function found for '%s'",pScript->GetScriptInfo(),Script::FindChecksumName(Checksum)));

			
	}

	return true;
}



uint32 CObject::GetFlags( Script::CScriptStructure *pParams, Script::CScript *pScript ) const
{
	
	uint32 Flags = 0;
	int Flag = 0;

	// Scan through any array of flags specified.
	Script::CArray *pArray=NULL;
	pParams->GetArray(NONAME,&pArray);
	if (pArray)
	{
		for (uint32 i=0; i<pArray->GetSize(); ++i)
		{
			uint32 Checksum=pArray->GetNameChecksum(i);
			int Flag=Script::GetInteger(Checksum);
			Dbg_MsgAssert(Flag>=0 && Flag<32,("\n%s\nBad flag value %s=%d, value must be between 0 and 31",pScript->GetScriptInfo(),Script::FindChecksumName(Checksum),Flag));
			Flags |= ( 1 << Flag );
		}	
	}
	else if ( pParams->GetInteger( NONAME, &Flag ) )
	{
		Dbg_MsgAssert(Flag>=0 && Flag<32,("\n%s\nBad flag value of %d, value must be between 0 and 31",pScript->GetScriptInfo(),Flag));
		Flags = ( 1 << Flag );
	}
	return Flags;
}

/*
	If an event is targeted specifically to this CObject (using the same ID), it will be
	passed through this function.
*/
bool CObject::PassTargetedEvent(CEvent *pEvent, bool broadcast)
{
#ifdef	__SCRIPT_EVENT_TABLE__		
	if (!mp_script)
	{
		return true;
	}
#endif
	
	Obj::CSmtPtr<CObject> p = this;
	
#ifdef	__SCRIPT_EVENT_TABLE__		
	mp_script->PassTargetedEvent(pEvent, broadcast);
#else	
	if (mp_event_handler_table)
		mp_event_handler_table->pass_event(pEvent, this, broadcast);
#endif

	return (p != NULL);
}


// Send myself an event.  Just a useful shortcut for a commonly done thing
void CObject::SelfEvent(uint32 event, Script::CStruct* pParams)
{
	Obj::CTracker::Instance()->LaunchEvent(event,GetID(),GetID(),pParams); 
}


void CObject::BroadcastEvent(uint32 event, Script::CStruct *pData, float radius)
{
	// no target
	// radius not implemented yet
	Obj::CTracker::Instance()->LaunchEvent( event, 0xffffffff, GetID(), pData, true /*, radius */  );
}



/*
	Has the same effect as deleting the CObject, except the actual deletion is deferred until next frame.
	So that objects can be killed and not mess up list traversal
*/
void CObject::MarkAsDead( void )
{
	
	// make sure we don't continue running anything on this script!
	if ( mp_script )
	{
		mp_script->Block( );
	}

	// The following line will set the mp_object of any referring scripts to NULL
	Script::StopAllScriptsUsingThisObject(this);  // Mick: we don't want any other scripts to continue running on a dead object 	
	
	Dbg_MsgAssert( mp_manager,( "No object manager associated with this object..." ));
	// who locks an object?
	SetLockOff();

	
	// mp_manager will be cleared by UnregisterObject()
	CBaseManager *p_manager = mp_manager;
	p_manager->UnregisterObject( *this );
	p_manager->KillObject( *this );  // add object to kill list, to be purged next frame...	
	m_object_flags |= ( vDEAD );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CObject::allocate_tags_if_needed()
{
	if (!mp_tags) 
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		mp_tags = new Script::CStruct();
		Mem::Manager::sHandle().PopContext();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CObject::AllocateScriptIfNeeded()
{
	if (!mp_script) 
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
		mp_script = new Script::CScript();
		mp_script->SetScript(CRCD(0x3f5cdb8a,"empty_script"),NULL,this);
		Mem::Manager::sHandle().PopContext();
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CObject::SwitchScript( uint32 scriptChecksum, Script::CScriptStructure *pParams )
{
	
	if ( mp_script )
	{
		mp_script->SetScript( scriptChecksum, pParams, this );
	}
	else
	{
		mp_script = new Script::CScript;
		mp_script->SetScript( scriptChecksum, pParams, this );
		#ifdef __NOPT_ASSERT__
		mp_script->SetCommentString("Created in CObject::SwitchScript(...)");
		#endif
	}
	if ( !mp_script->GotScript( ) )
	{
		Dbg_MsgAssert( 0,( "Couldn't find script specified: %s", Script::FindChecksumName( scriptChecksum ) ));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CScript *CObject::SpawnScriptPlease( uint32 scriptChecksum, Script::CScriptStructure *pParams, int Id, bool pause_with_object )
{
	
	Script::CScript *pScript;
	pScript = Script::SpawnScript( scriptChecksum, pParams, 0, NULL, -1, Id, false, false, false, pause_with_object ); // K: The 0,NULL means no callback specified.
	pScript->mpObject = this;
	return pScript;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// spawn and immediatly run a script
// script will be deleted after it terminates
void CObject::SpawnAndRunScript ( uint32 ScriptChecksum, int node, bool net_script, bool permanent, Script::CStruct *p_params )
{
    // Send this event to other clients if applicable
	if( net_script )
	{
		// Ken: TODO: Currently p_params are not being passed on in net games, fix if necessary ...
		// Currently p_params is only used to store the contents of any ModelTriggerScriptParams
		// that may be specified in an object's node and which are required to be passed on to
		// any TriggerScript in it's local node array. (Used by a goal in London)
		// (Note: The NULL below means a NULL p_object)
		Script::SendSpawnScript( ScriptChecksum, NULL, node, permanent );
	}

	Script::CScript* pScript = Script::SpawnScript( ScriptChecksum, p_params, 0, NULL, node );
	
	#ifdef __NOPT_ASSERT__
	pScript->SetCommentString( "Spawned by CObject::SpawnAndRun(...)" );
	#endif
	
	pScript->mpObject = this; 
	
	pScript->Update(); 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CObject::SpawnAndRunScript( const char *pScriptName, int node, bool net_script, bool permanent, Script::CStruct *p_params )
{
	SpawnAndRunScript(Script::GenerateCRC(pScriptName),node, net_script, permanent, p_params );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CObject::CallScript( uint32 ScriptChecksum, Script::CStruct *pParams )
{
	if ( !mp_script )
	{
		mp_script = new Script::CScript;
		
		// K: Added this in case the script being called contains member functions, which
		// it does when a two player trick-attack game is started.
		mp_script->mpObject=this;
	}

	mp_script->Interrupt(ScriptChecksum, pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			DestroyIfUnlocked( Obj::CObject* obj, void* data )
{
	obj->DestroyIfUnlocked();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			SetLockOff( CObject* obj, void* data )
{
	obj->SetLockOff();
}

void			CObject::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
		// CObject stuff
	Script::CStruct *p_scripts=new Script::CStruct;
	Script::CScript *p_script=Script::GetNextScript(NULL);
	while (p_script)
	{
		if (p_script->mpObject==this)
		{
			Script::CStruct *p_script_info=new Script::CStruct;
			
			#ifdef __NOPT_ASSERT__
			// Convert to microseconds by dividing by 150
			p_script_info->AddInteger("CPUTime",p_script->m_last_time/150);
			#endif
			
			p_script_info->AddChecksum("m_unique_id",p_script->GetUniqueId());
			if (p_script->mIsSpawned)
			{
				p_script_info->AddChecksum(NONAME,CRCD(0xf697fda7,"Spawned"));
			}
			
			#ifdef	__SCRIPT_EVENT_TABLE__		
				if (p_script->GetEventHandlerTable())
				{
					p_script->GetEventHandlerTable()->GetDebugInfo(p_script_info);
				}
			#endif
			
			
			p_scripts->AddStructurePointer(p_script->GetBaseScript(),p_script_info);
		}
			
		p_script=Script::GetNextScript(p_script);
	}	
	p_info->AddStructurePointer("Scripts",p_scripts);
	
	p_info->AddChecksum("m_id",m_id);
	if (mp_tags)
	{
		p_info->AddStructure("mp_tags",mp_tags);
	}
	else
	{
		p_info->AddChecksum("mp_tags",CRCD(0xda3403b0,"NULL"));
	}
	
#ifndef	__SCRIPT_EVENT_TABLE__		
	if (mp_event_handler_table)
	{
		mp_event_handler_table->GetDebugInfo(p_info);
	}
#endif
#endif				 

}

// Leftover from when this function was more complex
// just retained for convenience
CObject *ResolveToObject(uint32 id)
{
	return Obj::CTracker::Instance()->GetObject(id); 	
}


} // namespace Obj

