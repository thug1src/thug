#include <core/defines.h>
#include <core/math.h>
#include <objects/pathman.h>
#include <gel/object/compositeobject.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <sk/scripting/nodearray.h>

namespace Obj
{

CPathObjectTracker *CPathMan::TrackObject(CCompositeObject *p_ob, int node_number)
{
	Script::CStruct *p_node=SkateScript::GetNode(node_number);
	#ifdef __NOPT_ASSERT__
	Dbg_MsgAssert(p_node,("NULL p_node for node %d",node_number));
	#endif
	
	int path_number=0;
	if (!p_node->GetInteger("PathNum",&path_number))
	{
		Dbg_MsgAssert(0,("Node %d does not contain a PathNum",node_number));
	}
		
	Dbg_MsgAssert(path_number>=0 && path_number<MAX_PATHS,("Bad PathNum of %d for node %d",path_number,node_number));
	mp_path_object_trackers[path_number].AddObjectPointer(p_ob);
	return &mp_path_object_trackers[path_number];
}

CPathObjectTracker *CPathMan::TrackPed(CCompositeObject *p_ob, int node_number)
{
	Script::CStruct *p_node=SkateScript::GetNode(node_number);
	Dbg_MsgAssert(p_node,("NULL p_node for node %d",node_number));
	
	int path_number=0;
	if (!p_node->GetInteger("PathNum",&path_number))
	{
		Dbg_MsgAssert(0,("Node %d does not contain a PathNum",node_number));
	}
		
	// Dbg_MsgAssert(path_number>=0 && path_number<MAX_PATHS,("Bad PathNum of %d for node %d",path_number,node_number));
	// mp_path_object_trackers[path_number].AddObjectPointer(p_ob);
	mp_ped_object_tracker.AddObjectPointer( p_ob );
	return &mp_ped_object_tracker;
}

CPathObjectTracker::CPathObjectTracker()
{
	Clear();
}

CPathObjectTracker::~CPathObjectTracker()
{
}

void CPathObjectTracker::Clear()
{
	for (int i=0; i<MAX_OBJECTS_PER_PATH; ++i)
	{
		mpp_objects[i]=NULL;
	}
}

void CPathObjectTracker::StopTrackingObject(CCompositeObject *p_ob)
{
	Dbg_MsgAssert(p_ob,("NULL p_ob"));
	for (int i=0; i<MAX_OBJECTS_PER_PATH; ++i)
	{
		if (mpp_objects[i]==p_ob)
		{
			mpp_objects[i]=NULL;
		}
	}
}			

void CPathObjectTracker::AddObjectPointer(CCompositeObject *p_ob)
{
	Dbg_MsgAssert(p_ob,("NULL p_ob"));
	//printf("Tracking %s\n",Script::FindChecksumName(p_ob->mNodeNameChecksum));

	for (int i=0; i<MAX_OBJECTS_PER_PATH; ++i)
	{
		if (!mpp_objects[i])
		{
			mpp_objects[i]=p_ob;
			return;
		}
	}		
	Dbg_MsgAssert(0,("Full CPathObjectTracker"));
}


CPathMan *CPathMan::mp_instance=NULL;

CPathMan *CPathMan::Instance()
{
	if (!mp_instance)
	{
		mp_instance=new CPathMan;
	}
	return mp_instance;
}

CPathMan::CPathMan()
{
	m_num_paths=0;
	mp_path_object_trackers = NULL;
}

void CPathMan::ClearPathObjectTrackers()
{
	for (int i=0; i<MAX_PATHS; ++i)
	{
		mp_path_object_trackers[i].Clear();
	}	
}

// Called just after a new NodeArray is loaded, in ScriptLoadNodeArray in cfuncs.cpp
// Scans through the node array inserting PathNum members to all nodes that are on a path.
// Enables quick look up of what path a car is on, & hence whether it is on the same path as
// another car.
void CPathMan::AddPathInfoToNodeArray()
{
//	Dbg_Assert( mp_path_object_trackers );
	if (!mp_path_object_trackers)
	{
		return;
	}
	
	// printf("CPathMan::AddPathInfoToNodeArray\n");
	Script::CArray *p_node_array=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"),Script::ASSERT);
	
	ClearPathObjectTrackers();
	
	// The path number assigned to the nodes is just an integer, starting at 0   
	m_num_paths=0;
	
	int array_size=p_node_array->GetSize();
	for ( uint16 i = 0; i < array_size; ++i )
	{
		Script::CStruct *p_start_node=p_node_array->GetStructure( i );
		
		// Rail nodes are linked into a path, but don't need path numbers, so don't bother
		// adding them to save memory.
		uint32 class_type=0;
		p_start_node->GetChecksum( CRCD(0x12b4e660,"Class"), &class_type );
		if ( class_type == CRCD( 0x8e6b02ad, "RailNode" ) ||
			 class_type == CRCD(0x30c19600, "ClimbingNode") )
		{
			continue;
		}

		// make sure this isn't a pedai waypoint
		uint32 type;
		if ( class_type == CRCD( 0x4c23a77e, "Waypoint" )
			 && p_start_node->GetChecksum( CRCD( 0x7321a8d6, "type" ), &type, Script::NO_ASSERT )
			 && type == CRCD( 0xcba10ffa, "PedAI" ) )
		{
			continue;
		}
		
		int num_links=SkateScript::GetNumLinks( p_start_node );
		int node_path_number = 0;
		// Check if the node is linked to something, and has not already had a PathNum
		// given to it.
		if ( ( num_links || class_type == CRCD( 0x4c23a77e, "WayPoint" ) ) && 
			!p_start_node->ContainsComponentNamed( CRCD( 0x9c91c3ca, "PathNum" ) ) )
		{
			// printf("found a non PedAI waypoint\n");
			// The node and subsequent nodes that it is linked to require a PathNum				
			Script::CStruct *p_node=p_start_node;
			
			// The path number that is going to be written in is m_num_paths to start with,
			// but that might change if we hit a node that already has a path number. In that
			// case we'll rewind and write that number in instead.
			int path_number_to_write=m_num_paths;
			
			while (true)
			{
				if (p_node->GetInteger( CRCD( 0x9c91c3ca, "PathNum" ),&node_path_number))
				{
					// We've hit a node that has already had a PathNum written in.
					
					if (node_path_number==path_number_to_write)
					{
						// The path num matches what we started with, so must have looped
						// back to the start, so finished.
						break;
					}	
					
					// We've hit a node that has a different PathNum. So we need to rewind
					// back to where we started and write in that path number instead.
					// This happens due to the fact that the nodes could be in any order in
					// the node array, so paths can start being traversed from any point, not
					// necessarily the start.
					path_number_to_write=node_path_number;
					p_node=p_start_node;
				}
				
				// Wack in the path number
				p_node->AddInteger(0x9c91c3ca/*PathNum*/,path_number_to_write);
				
				// Check if this is a terminating node of the path, and stop if so.
				int num_links=SkateScript::GetNumLinks(p_node);
				if (num_links==0)
				{
					break;
				}	
				
				// On to the next node. Note the if the path branches, it will only
				// follow the first branch. The other branches will end up getting
				// their own path numbers. Don't know if this will be a problem yet.
				p_node=p_node_array->GetStructure(SkateScript::GetLink( p_node, 0 ));
			}
			
			// If path_number got used, increment it ready for the next path.
			if (path_number_to_write==m_num_paths)
			{
				++m_num_paths;
				Dbg_MsgAssert(m_num_paths<MAX_PATHS,("Too many paths, max=%d",MAX_PATHS));
			}	
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathMan::DeallocateObjectTrackerMemory()
{
	if ( mp_path_object_trackers )
	{
		ClearPathObjectTrackers();
		delete[] mp_path_object_trackers;
		mp_path_object_trackers = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathMan::AllocateObjectTrackerMemory()
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	mp_path_object_trackers = new CPathObjectTracker[MAX_PATHS];
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptAllocatePathManMemory( Script::CStruct* pParams, Script::CScript* pScript )
{
	Obj::CPathMan* pPathMan = Obj::CPathMan::Instance();
	Dbg_Assert( pPathMan );
	pPathMan->AllocateObjectTrackerMemory();
	return true;
}

} // namespace Obj

