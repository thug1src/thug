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
**	File name:		objman.cpp												**
**																			**
**	Created:		05/27/99	-	mjb										**
**																			**
**	Description:	Object Manager											**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/singleton.h>

#include <core/list.h>
#include <core/task.h>

#include <gel/objman.h>
#include <gel/objtrack.h>
#include <gel/mainloop.h>

#include <gel/scripting/script.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

//#define __DEBUG_OBJ_MAN__

namespace Obj
{



/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

const uint32 CBaseManager::vNO_GROUP = 0;
const uint32 CBaseManager::vNO_OBJECT_ID		=	vUINT32_MAX;

const sint CBaseManager::vNO_OBJECT_TYPE	=	0;

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

void	CGeneralManager::flush_dead_objects( const Tsk::Task< CGeneralManager >& task )
{
	 

	CGeneralManager&		obj_manager = task.GetData();
	
	obj_manager.FlushDeadObjects();

}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseManager::CBaseManager( void )
: m_list_changed( 0x00000000 )
{
	m_momentary_removal = false;
}




/*
	Adds the specified CObject to the manager, registers it with the CTracker singleton.
	
	If the CObject has no ID, it is automatically assigned one at this point.
*/
void CBaseManager::RegisterObject ( CObject& obj )
{
	CTracker* p_tracker = CTracker::Instance();
	Dbg_MsgAssert(!obj.mp_manager, ("object already registered with manager"));

   	obj.mp_manager = this;
	m_object_list.AddNode ( &obj.m_node );
	
	// set all list changed bits
	m_list_changed = 0xFFFFFFFF;
	
	if (obj.GetID() == vNO_OBJECT_ID)
		obj.SetID(p_tracker->GetFreshId());

	p_tracker->addObject(&obj);
}




/*
	Removes the specified CObject to the manager, unregisters it from the CTracker singleton.
	
	After this, the CObject is no longer "in the system", and probably should be deleted. Its
	ID will be available for usage by other CObjects again.
*/
void CBaseManager::UnregisterObject( CObject& obj )
{
	CTracker* p_tracker = CTracker::Instance();
	Dbg_MsgAssert(obj.mp_manager == this, ("object not registered with this manager"));
	Dbg_AssertType( &obj, CObject );

	p_tracker->removeObject(&obj, m_new_id_of_object_being_momentarily_removed, m_momentary_removal);
	
	if ( obj.m_node.InList() )
	{
		obj.m_node.Remove();
		obj.mp_manager = NULL;

		// set all list changed bits
		m_list_changed = 0xFFFFFFFF;
	}
}




/*
	Called by CObject::SetID() when a CObject's ID is changed. Shouldn't be called
	from any other place.
*/
void CBaseManager::ReregisterObject(CObject& obj, uint32 newId)
{
	/* 
		In the CTracker singleton, we are concerned with keeping:
		-Aliases. They don't use object ID, just pointers
		-Event listeners: don't use ID, so no worries
		-Waiting script entries: scan through and change stored ID
	*/
	
	m_new_id_of_object_being_momentarily_removed = newId;
	m_momentary_removal = true;
	UnregisterObject(obj);
	obj.m_id = newId;
	RegisterObject(obj);
	m_momentary_removal = false;
	
}

/*
	Changes the priority of an object, then reregisters.
*/
void CBaseManager::SetObjectPriority ( CObject& obj, Lst::Node< CObject >::Priority priority )
{
	obj.m_node.SetPri(priority);
	ReregisterObject(obj, obj.GetID());
}






/*
	Deletes CObjects that have been killed by CObject::mark_as_dead() (which is called
	by the "Die" member command). These CObjects will be "out of play" by this point, and
	only need be deleted.
*/
void CGeneralManager::FlushDeadObjects()
{
	Lst::Head< CObject >*		head = &m_kill_list;
	Lst::Node< CObject >*		next = head->GetNext();

	while ( next )
	{
		Lst::Node< CObject >*	kill = next;
		next = next->GetNext();
		kill->Remove();
		CObject *pObject = kill->GetData( );
		#ifdef __NOPT_ASSERT__
		// Lock Assert is not needed for objects that have been flagged for deletion  	
		pObject->SetLockAssertOff();
		#endif
		delete pObject;
	}	
}


CGeneralManager::CGeneralManager( void )
{	
	// add flush task
	mp_kill_task = new Tsk::Task< CGeneralManager > ( flush_dead_objects, *this, Tsk::BaseTask::Node::vSYSTEM_TASK_PRIORITY_FLUSH_DEAD_OBJECTS );
	Dbg_AssertType ( mp_kill_task, Tsk::Task< CGeneralManager > );

	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	mlp_man->AddSystemTask ( *mp_kill_task ); 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeneralManager::~CGeneralManager( void )
{
	UnlockAllObjects();
	DestroyAllObjects();
		
	Dbg_AssertType ( mp_kill_task, Tsk::Task< CGeneralManager > );
	delete mp_kill_task;
}




/*
	Runs the specified function on all CObjects of the given type. Deleting is allowed.
*/
void			CGeneralManager::ProcessAllObjectsOfType( sint type, Callback* process, void* data )
{
	Lst::Search< CObject >	sh;
	
	uint32 stamp_mask = m_stamp_bit_manager.RequestBit();
	
	// clear the appropriate stamp bit
	for (CObject* pObject = sh.FirstItem(m_object_list); pObject; pObject = sh.NextItem())
	{
		pObject->ClearStampBit(stamp_mask);
	}
	
	// traverse the object list
	do
	{
		// clear the appropriate list changed bit
		m_list_changed &= ~stamp_mask;
		
        CObject* pNextObject = sh.FirstItem(m_object_list);
		
		while (pNextObject)
		{
			CObject* pObject = pNextObject;
			pNextObject = sh.NextItem();
			
			if (pObject->GetType() == type && !pObject->CheckStampBit(stamp_mask))
			{
				pObject->SetStampBit(stamp_mask);
				
				process(pObject, data);
				
				if (m_list_changed & stamp_mask) break;
			}
		}
	}
	while (m_list_changed & stamp_mask);
	
	m_stamp_bit_manager.ReturnBit(stamp_mask);
}




/*
	Runs the specified function on all CObjects in manager. Deleting is allowed.
*/
void			CGeneralManager::ProcessAllObjects( Callback* process, void *data )
{
	Lst::Search< CObject >	sh;
	
	uint32 stamp_mask = m_stamp_bit_manager.RequestBit();
	
	// clear the appropriate stamp bit
	for (CObject* pObject = sh.FirstItem(m_object_list); pObject; pObject = sh.NextItem())
	{
		pObject->ClearStampBit(stamp_mask);
	}
	
	// traverse the object list
	do
	{
		// clear the appropriate list changed bit
		m_list_changed &= ~stamp_mask;
		
        CObject* pNextObject = sh.FirstItem(m_object_list);
		
		while (pNextObject)
		{
			CObject* pObject = pNextObject;
			pNextObject = sh.NextItem();
			
			if (!pObject->CheckStampBit(stamp_mask))
			{
				pObject->SetStampBit(stamp_mask);
				
				process(pObject, data);
				
				if (m_list_changed & stamp_mask) break;
			}
		}
	}
	while (m_list_changed & stamp_mask);
	
	m_stamp_bit_manager.ReturnBit(stamp_mask);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

sint			CGeneralManager::CountObjectsOfType( sint type )
{
	

	sint					count = 0;
	CObject*					obj;
	Lst::Search< CObject >	sh;


	obj = sh.FirstItem( m_object_list );

	while ( obj )
	{
		Dbg_AssertType( obj, CObject );

		if (obj->GetType() == type)
		{
			count++;
		}
		obj = sh.NextItem();
	}

	return count;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeneralManager::AssertIfObjectsRemainApartFrom( sint *pApartFromThisType )
{
	

	#ifdef __NOPT_ASSERT__
	
	CObject* pObj;
	Lst::Search< CObject > sh;

	bool GotRemainingObjects=false;
	
	pObj = sh.FirstItem( m_object_list );

	while ( pObj )
	{
		Dbg_AssertType( pObj, CObject );

		bool Match=false;
		sint *pType=pApartFromThisType;
		while (*pType!=-1)
		{
			if (pObj->GetType()==*pType)
			{
				Match=true;
				break;
			}	
			++pType;
		}	
		
		if ( !Match )
		{
			printf("Object type = %d\n",pObj->GetType());
			GotRemainingObjects=true;
		}
		pObj = sh.NextItem();
	}

	Dbg_MsgAssert(!GotRemainingObjects,("Objects remain !!!"));
	
	#endif
}




/*
	Called by CObject::mark_as_dead(). Shouldn't call from anywhere else.
*/
void		CGeneralManager::KillObject( CObject& obj )
{	
	Dbg_AssertType( &obj, CObject );
	//Dbg_MsgAssert( obj.mp_node, ( "CObject does not have a node" ));
	Dbg_MsgAssert( !obj.m_node.InList(),( "CObject has not been unregistered" ));
	
	m_kill_list.AddToHead( &obj.m_node );
}




/*
	Returns a (currently) unused ID.
*/
uint32 CGeneralManager::NewObjectID( void )
{
	CTracker* p_tracker = CTracker::Instance();
	return p_tracker->GetFreshId();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CObject*		CGeneralManager::GetObjectByID( uint32 id )
{
	CTracker* p_tracker = CTracker::Instance();
	return p_tracker->GetObject(id);	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj

