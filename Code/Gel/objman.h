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
**	File name:		gel/objman.h											**
**																			**
**	Created: 		05/27/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef __GEL_OBJMAN_H
#define __GEL_OBJMAN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/list.h>
#include <core/task.h>
#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Lst
{
	template< class _V > class HashTable;
}



namespace Obj
{



/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

	
	
	
/*
	This class manages usage of the bits of a uint32.  Use of CBitManager insures that no two sections of the greater code are using the same bit.  When
	a bit is requested from a CBitManager, no other requests for bits will give that bit until access to the bit has been returned to the manager.
	Bit requests and returns are done in the form of uint32 masks.
*/
class CBitManager
{
public:
								CBitManager (   );
	
	uint32						RequestBit (   );
	void						ReturnBit ( uint32 mask );
	
private:
	uint32						m_used_bit_mask;
};




/*
	See document for more info on this class: Q:\sk4\Docs\programming\Object Scripting System.htm
	
	The base class for all object managers. An object manager keeps track of a related
	set of CObjects (e.g. the screen element manager manages all CScreenElements).
	The manager might also be responsible for creation and destruction of its CObjects.
	
	An subclass of CBaseMananger must register its CObjects with the CTracker singleton.
	
	This is a virtual base class.
*/
class CBaseManager : public Spt::Class
{
public:	
	
	static const uint32			vNO_GROUP;
	static const uint32			vNO_OBJECT_ID;
								
	static const sint			vNO_OBJECT_TYPE;

								CBaseManager();
	virtual 					~CBaseManager() {;}
	
	virtual void				RegisterObject ( CObject& obj );
	virtual void				UnregisterObject ( CObject& obj );
	virtual void				ReregisterObject ( CObject& obj, uint32 newId );
	
	virtual void				SetObjectPriority ( CObject& obj, Lst::Node< CObject >::Priority priority );
	
	// Called by CObject::mark_as_dead()
	virtual void				KillObject ( CObject& obj ) = 0;
	
	virtual Lst::Head< CObject > &GetRefObjectList() = 0;

protected:

	Lst::Head< CObject >		m_object_list;	// list of created objects
	
	CBitManager					m_stamp_bit_manager;
	
	uint32						m_list_changed;

	// important in ReregisterObject(); if applicable, set to ID of new object,
	// otherwise, zero
	uint32						m_new_id_of_object_being_momentarily_removed;
	bool						m_momentary_removal;
};




/*
	See document for more info on this class: Q:\sk4\Docs\programming\Object Scripting System.htm
	
	The generic implementation of CBaseManager. Use when you want a very
	basic manager only.
*/
class  CGeneralManager  : public CBaseManager
{
	

public :
	
	typedef	void (Callback) ( CObject*, void *data );

								CGeneralManager ( void );
								~CGeneralManager ( void );

	void						ProcessAllObjects ( Callback* process, void* data = NULL );
	void						ProcessAllObjectsOfType ( sint type, Callback* process, 
														  void* data = NULL );

	void						DestroyAllObjects ( void );
	void						DestroyAllObjectsOfType ( sint type );

	void						UnlockAllObjects ( void );
	void						UnlockAllObjectsOfType ( sint type );
	

	void						KillObject ( CObject& obj );

	uint32						NewObjectID( void );
	sint						CountObjectsOfType ( sint type );
	void 						AssertIfObjectsRemainApartFrom( sint *pApartFromThisType );


	void 						FlushDeadObjects();

	CObject*					GetObjectByID( uint32 id );
	
	Lst::Head< CObject > &		GetRefObjectList() {return m_object_list;}

protected :
	
	static Tsk::Task< CGeneralManager >::Code	flush_dead_objects;

	Lst::Head< CObject >		m_kill_list;		// list of objects to be killed
	Tsk::Task< CGeneralManager >*		mp_kill_task;		// kill task that processes the kill list

//	Lst::HashTable<CObject> *	mp_hash_table;
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

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline				CBitManager::CBitManager (   )
	:	m_used_bit_mask(0x00000000)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	uint32		CBitManager::RequestBit (   )
{
	Dbg_MsgAssert(m_used_bit_mask != 0xFFFFFFFF, ("Out of available bits in CBitManager::RequestBit"));
	
	uint32 ready_mask = nBit(0);
	while (m_used_bit_mask & ready_mask)
	{
		ready_mask <<= 1;
	}
	
	m_used_bit_mask |= ready_mask;
	
	return ready_mask;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		CBitManager::ReturnBit ( uint32 returning_mask )
{
	Dbg_MsgAssert((returning_mask & (returning_mask - 1)) == 0, ("Calling CBitManager::ReturnBit with a mask contining more than a single set bit"));
	Dbg_MsgAssert(returning_mask & m_used_bit_mask, ("Returning an unused bit to CBitManager::ReturnBit"));
	
	m_used_bit_mask &= ~returning_mask;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		CGeneralManager::DestroyAllObjects ( void )
{
   	
	
	ProcessAllObjects ( DestroyIfUnlocked );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline	void		CGeneralManager::DestroyAllObjectsOfType ( sint type )
{
   	

	ProcessAllObjectsOfType ( type, DestroyIfUnlocked );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		CGeneralManager::UnlockAllObjects ( void )
{
   	

	ProcessAllObjects ( SetLockOff, NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline	void		CGeneralManager::UnlockAllObjectsOfType ( sint type )
{
   	

	ProcessAllObjectsOfType ( type, SetLockOff );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj

#endif	// __GEL_OBJMAN_H
