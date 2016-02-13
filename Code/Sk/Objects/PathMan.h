/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Object (OBJ)											**
**																			**
**	File name:		objects/pathman.h    									**
**																			**
**	Created: 		5/08/02	-	ksh											**
**																			**
*****************************************************************************/

#ifndef __OBJECTS_PATHMAN_H
#define __OBJECTS_PATHMAN_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

#ifndef __GEL_OBJPTR_H
#include <gel/objptr.h>
#endif

#include <gel/scripting/struct.h>
#include <gel/scripting/script.h>

namespace Obj
{
class CCompositeObject;

class CPathObjectTracker
{
public:	
enum
{
	MAX_OBJECTS_PER_PATH=50
};	
private:
	CSmtPtr<CCompositeObject> mpp_objects[MAX_OBJECTS_PER_PATH];
	
public:	
	CPathObjectTracker();
	~CPathObjectTracker();
	void Clear();
	void AddObjectPointer(CCompositeObject *p_ob);
	void StopTrackingObject(CCompositeObject *p_ob);

	const CSmtPtr<CCompositeObject> *GetObjectList() {return mpp_objects;}
};

class CPathMan
{
public:
	static CPathMan *Instance();
	void RecursivelyAddPathInfo( int path_number_to_write, int node_number, int start_node_number, int recursion_depth = 0 );
protected:
	CPathMan();
private:
	static CPathMan *mp_instance;		
	
	#define MAX_PATHS 300
	CPathObjectTracker* mp_path_object_trackers;
	CPathObjectTracker mp_ped_object_tracker;
	int m_num_paths;
	
public:
	void AddPathInfoToNodeArray();
	void ClearPathObjectTrackers();

	void DeallocateObjectTrackerMemory();
	void AllocateObjectTrackerMemory();
	
	CPathObjectTracker *TrackObject(CCompositeObject *p_ob, int node_number);
	CPathObjectTracker *TrackPed(CCompositeObject *p_ob, int node_number);
};

//char foo[sizeof(CPathMan)/0];
bool ScriptAllocatePathManMemory( Script::CStruct* pParams, Script::CScript* pScript );

} // namespace Obj

#endif

