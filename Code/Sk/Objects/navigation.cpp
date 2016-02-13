///////////////////////////////////////////////////////
// rail.cpp

#include <sk/objects/navigation.h>

#include <core/defines.h>
#include <core/singleton.h>
#include <core/math.h>
#include <core/math/geometry.h>

#include <gel/modman.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/utils.h>
#include <gel/environment/terrain.h>
#include <gel/collision/collision.h>

#include <gfx/nx.h>
#include <gfx/debuggfx.h>

#include <sk/engine/feeler.h>
#include <sk/modules/skate/skate.h>
#include <sk/scripting/nodearray.h>
		  

namespace Obj
{









/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sAStarOpenList
{
	CNavNode*	m_open_list[512];
	int			m_num_open;

				sAStarOpenList( void );
				~sAStarOpenList( void );

	bool		IsEmpty( void )				{ return m_num_open == 0; }
	void		Clear();
	void		Empty();
	void		Push( CNavNode* p_node );
	CNavNode*	Pop( void );
	void		SortOnTotalCost();
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sAStarClosedList
{
	CNavNode*	m_closed_list[512];
	int			m_num_closed;

				sAStarClosedList( void );
				~sAStarClosedList( void );

	bool		IsEmpty( void )				{ return m_num_closed == 0; }
	void		Clear();
	void		Empty( void );
	void		Add( CNavNode* p_node );
	void		Remove( CNavNode* p_node );
};



static sAStarOpenList	openList;
static sAStarClosedList	closedList;



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sAStarNode::Reset( void )
{
	m_flags	= 0;
	m_cost_from_start	= 0.0f;
	m_cost_to_goal		= 0.0f;
	m_total_cost		= 0.0f;
	mp_parent			= NULL;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool normal_sensitive_collision( CFeeler& feeler, float normal_y_sensitivity )
{
	// Safety check.
	int iteration = 0;

	// The feeler should already have been set up with the start and end points.
	while( feeler.GetCollision())
	{
		// If this collision is with a normal that passes ouyr sensitivity test, consider it valid.
		if( Mth::Abs( feeler.GetNormal()[Y] ) < normal_y_sensitivity )
		{
			return true;
		}

		// Otherwise move the start location on by 0.1 inch and continue.
		feeler.m_start += ( feeler.m_end - feeler.m_start ).Normalize( 0.1f );

		++iteration;
		if( iteration > 8 )
		{
			Dbg_Message( "Multiple iterations in normal_sensitive_collision()" );

			// Err on the side of caution.
			return true;
		}
	}
	return false;
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float path_cost_estimate( Mth::Vector& pos_a, Mth::Vector& pos_b )
{
	// Currently just use the distance between them. No consideration of terrain type.
	return Mth::Distance( pos_a, pos_b );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float traverse_cost( CNavNode* p_node_a, CNavNode* p_node_b )
{
	// Currently just use the distance between them. No consideration of terrain type.
	return Mth::Distance( p_node_a->GetPos(), p_node_b->GetPos());
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int sort_on_total_cost( const void *p1, const void *p2 )
{
	CNavNode*	p_node1	= *((CNavNode**)p1 );
	CNavNode*	p_node2	= *((CNavNode**)p2 );
	
	return ( p_node1->GetAStarNode()->m_total_cost < p_node2->GetAStarNode()->m_total_cost ) ? 1 : (( p_node1->GetAStarNode()->m_total_cost > p_node2->GetAStarNode()->m_total_cost ) ? -1 : 0 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNavNode* get_nearest_visible_node( Mth::Vector& pos, Mth::Vector& heading )
{
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Obj::CNavManager* p_nav_man = skate_mod->GetNavManager();

	float		best_dp		= -0.707106f;	// cosine of 135 degrees
	CNavNode*	p_best_node	= NULL;

	int num_nodes = p_nav_man->GetNumNodes();
	for( int i = 0; i < num_nodes; ++i )
	{
		CNavNode*	p_node = p_nav_man->GetNavNodeByIndex( i );

		// Discard nodes that are not in the required direction.
		Mth::Vector node_pos	= p_node->GetPos() + Mth::Vector( 0.0f, 60.0f, 0.0f, 0.0f );
		Mth::Vector pos_to_node	= node_pos - pos;
		pos_to_node[Y]			= 0.0f;
		pos_to_node[W]			= 0.0f;
		pos_to_node.Normalize();
		float dp				= Mth::DotProduct( pos_to_node, heading );

		// Do we already have a better node.
		if( dp < best_dp )
		{
			continue;
		}

		// Discard nodes that are not visible.
		CFeeler feeler;
		feeler.m_start	= pos;
		feeler.m_end	= node_pos;
		if( normal_sensitive_collision( feeler, 0.2f ))
		{
			continue;
		}

		best_dp		= dp;
		p_best_node	= p_node;
	}
	return p_best_node;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNavNode* CalculateNodePath( Mth::Vector pos, Mth::Vector target_pos )
{
	// Clear open and closed nodes.
	openList.Clear();
	closedList.Clear();

	// Find nearest visible node from current location, preferably in direction currently facing.
	Mth::Vector heading = target_pos - pos;
	heading[Y] = 0.0f;
	heading[W] = 0.0f;
	heading.Normalize();
	CNavNode* p_nearest_visible = get_nearest_visible_node( pos, heading );
	if( p_nearest_visible == NULL )
	{
		return false;
	}

	// Initialise the A* section of the node.
	p_nearest_visible->GetAStarNode()->m_cost_from_start	= path_cost_estimate( pos, p_nearest_visible->GetPos());
	p_nearest_visible->GetAStarNode()->m_cost_to_goal		= path_cost_estimate( p_nearest_visible->GetPos(), target_pos );
	p_nearest_visible->GetAStarNode()->m_total_cost			= p_nearest_visible->GetAStarNode()->m_cost_from_start + p_nearest_visible->GetAStarNode()->m_cost_to_goal;
	p_nearest_visible->GetAStarNode()->mp_parent			= NULL;

	// Push this node on to the open list.
	openList.Push( p_nearest_visible );

	while( !openList.IsEmpty())
	{
		// Pop lowest total cost node from open list.
		CNavNode* p_this_node = openList.Pop();

		// If goal visible from this node, we're done.
		Mth::Vector raised_node_pos = p_this_node->GetPos() + Mth::Vector( 0.0f, 60.0f, 0.0f, 0.0f );
		CFeeler feeler;
		feeler.m_start	= raised_node_pos;
		feeler.m_end	= target_pos;
		if( !normal_sensitive_collision( feeler, 0.2f ))
		{
			// Build list of nodes.

			// Empty lists.
			openList.Empty();
			closedList.Empty();

			return p_this_node;
		}

		// For each node linked to this node...
		for( int l = 0; l < p_this_node->GetNumLinks(); ++l )
		{
			CNavNode*	p_new_node	= p_this_node->GetLink( l );
			float		new_cost	= p_this_node->GetAStarNode()->m_cost_from_start + traverse_cost( p_this_node, p_new_node );

			// Ignore this node if it is already in a list and provides no improvement.
			if(( p_new_node->GetAStarNode()->InOpen() || p_new_node->GetAStarNode()->InClosed()) &&
				 p_new_node->GetAStarNode()->m_cost_from_start <= new_cost )
			{
				continue;
			}

			// Store the new or improved information.
			p_new_node->GetAStarNode()->mp_parent			= p_this_node;
			p_new_node->GetAStarNode()->m_cost_from_start	= new_cost;
			p_new_node->GetAStarNode()->m_cost_to_goal		= path_cost_estimate( p_new_node->GetPos(), target_pos );
			p_new_node->GetAStarNode()->m_total_cost		= p_new_node->GetAStarNode()->m_cost_from_start + p_new_node->GetAStarNode()->m_cost_to_goal;

			// Remove new node from closed list if it's in there.
			if( p_new_node->GetAStarNode()->InClosed())
			{
				closedList.Remove( p_new_node );
			}

			// Push new node onto open list (if it's not already in there).
			// This will re-sort the open list.
			if( !p_new_node->GetAStarNode()->InOpen())
			{
				openList.Push( p_new_node );
			}
		}

		// Add node onto the closed list.
		closedList.Add( p_this_node );
	}

	// Empty lists.
	openList.Empty();
	closedList.Empty();

	// If we get here we have failed to find any valid path.
	return NULL;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sAStarOpenList::sAStarOpenList( void )
{
	Clear();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sAStarOpenList::~sAStarOpenList( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sAStarOpenList::Clear( void )
{
	memset( m_open_list, 0, sizeof( m_open_list ));
	m_num_open = 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sAStarOpenList::Empty( void )
{
	while( !IsEmpty())
	{
		Pop();
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sAStarOpenList::Push( CNavNode* p_node )
{
	Dbg_Assert( m_num_open < 512 );

	m_open_list[m_num_open++] = p_node;

	// Flag this node as now being in the open list.
	p_node->GetAStarNode()->m_flags |= sAStarNode::IN_OPEN;

	SortOnTotalCost();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNavNode* sAStarOpenList::Pop( void )
{
	Dbg_Assert( m_num_open > 0 );

	// Assumes always sorted, so just return lowest cost node.
	--m_num_open;
	CNavNode* p_return		= m_open_list[m_num_open];
	m_open_list[m_num_open]	= NULL;

	// Flag this node as no longer being in the open list.
	p_return->GetAStarNode()->m_flags &= ~sAStarNode::IN_OPEN;

	return p_return;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sAStarOpenList::SortOnTotalCost( void )
{
	if( m_num_open > 1 )
	{
		qsort( m_open_list, m_num_open, sizeof( CNavNode* ), sort_on_total_cost );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sAStarClosedList::sAStarClosedList( void )
{
	Clear();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sAStarClosedList::~sAStarClosedList( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sAStarClosedList::Clear( void )
{
	memset( m_closed_list, 0, sizeof( m_closed_list ));
	m_num_closed = 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sAStarClosedList::Empty( void )
{
	if( !IsEmpty())
	{
		for( int i = 0; i < 512; ++i )
		{
			if( m_closed_list[i] )
			{
				Remove( m_closed_list[i] );
			}
		}
		Dbg_Assert( IsEmpty());
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sAStarClosedList::Add( CNavNode* p_node )
{
	for( int i = 0; i < 512; ++i )
	{
		if( m_closed_list[i] == NULL )
		{
			m_closed_list[i] = p_node;
			++m_num_closed;
			p_node->GetAStarNode()->m_flags |= sAStarNode::IN_CLOSED;
			return;
		}
	}
	Dbg_Assert( 0 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sAStarClosedList::Remove( CNavNode* p_node )
{
	for( int i = 0; i < 512; ++i )
	{
		if( m_closed_list[i] == p_node )
		{
			m_closed_list[i] = NULL;
			--m_num_closed;
			p_node->GetAStarNode()->m_flags &= ~sAStarNode::IN_CLOSED;
			return;
		}
	}
	Dbg_Assert( 0 );
}




































/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNavManager::CNavManager()
{
	mp_nodes		= NULL;
	m_num_nodes		= 0;
	mp_node_array	= NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNavManager::~CNavManager()
{	 
	Cleanup();  
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNavManager::Cleanup()
{
	m_num_nodes = 0;
	
	Mem::Free( mp_nodes );
	mp_nodes		= NULL;
	mp_node_array	= NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNavManager::AddNavNode( int node_number, Script::CStruct* p_node_struct )
{
	CNavNode* pNavNode = &mp_nodes[m_current_node];
	
	uint32 class_checksum;
	p_node_struct->GetChecksum( CRCD( 0x12b4e660, "class" ), &class_checksum );
	
	uint32 type_checksum;
	p_node_struct->GetChecksum( CRCD( 0x7321a8d6, "type" ), &type_checksum );

	// No links as yet.
	pNavNode->m_num_links = 0;
	for( int i = 0;i < MAX_NAV_LINKS; ++i )
	{
		pNavNode->mp_links[i] = NULL;
	}

	// The node_number is use primarily for calculating links.
	pNavNode->m_node		= node_number;
	
	// Reset the A* node.
	pNavNode->m_astar_node.Reset();
	
//	pNavNode->m_flags	= 0;
//	pNavNode->SetActive( p_node_struct->ContainsFlag( 0x7c2552b9 /*"CreatedAtStart"*/ ));	// Created at start or not?

	// Set the position from the node structure.
	SkateScript::GetPosition( p_node_struct, &pNavNode->m_pos );

	int links = SkateScript::GetNumLinks( p_node_struct );
	if( links )
	{
		Dbg_MsgAssert( links <= MAX_NAV_LINKS, ( "Nav node %d has %d linkes, max is %d", node_number, links, MAX_NAV_LINKS ));

		pNavNode->m_num_links = links;
		for( int i = 0; i < links; ++i )
		{
			// For now we just set the link to be the node number in the node array.
			pNavNode->mp_links[i] = (CNavNode*)SkateScript::GetLink( p_node_struct, i );
		}
	}	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNavManager::AddNavsFromNodeArray( Script::CArray *p_nodearray )
{
	Dbg_MsgAssert( !mp_node_array, ( "Setting two node arrays in rail manager" ));
	mp_node_array = p_nodearray;
	
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();

	uint32 i;	

	Dbg_MsgAssert( m_num_nodes == 0, ("Can only addd nodes once, already %d there\n", m_num_nodes ));

	// First iterate over the node array, and count the number of nodes needed.
	m_num_nodes = 0;										  
	for( i = 0; i < p_nodearray->GetSize(); ++i )
	{
		Script::CStruct* p_node_struct = p_nodearray->GetStructure( i );
		Dbg_MsgAssert( p_node_struct, ( "Error getting node from node array for navigation" ));
		if( !skate_mod->ShouldBeAbsentNode( p_node_struct ))
		{
			uint32 class_checksum = 0;		
			p_node_struct->GetChecksum( CRCD( 0x12b4e660, "Class" ), &class_checksum );
			if( class_checksum == CRCD( 0x4c23a77e, "Waypoint" ))
			{
				uint32 type_checksum = 0;		
				p_node_struct->GetChecksum( CRCD( 0x7321a8d6, "Type" ), &type_checksum );
				if( type_checksum == CRCD( 0x9784feb5, "HorseNav" ))
				{
					++m_num_nodes;
				}
			}
		}
	}

	// No nodes found, so just return
	if( m_num_nodes == 0 )
	{
		return;
	}
	
	mp_nodes = (CNavNode*)Mem::Malloc( m_num_nodes * sizeof( CNavNode ));

	// Iterate over the nodes and add them to the array.
	m_current_node = 0;	
	
	for( i = 0; i < p_nodearray->GetSize(); ++i )
	{
		Script::CStruct *p_node_struct = p_nodearray->GetStructure( i );
		Dbg_MsgAssert( p_node_struct, ("Error getting node from node array for navigation" ));

		if( !skate_mod->ShouldBeAbsentNode( p_node_struct ))
		{
			uint32 class_checksum = 0;		
			p_node_struct->GetChecksum( CRCD( 0x12b4e660, "Class" ), &class_checksum );
			if( class_checksum == CRCD( 0x4c23a77e, "Waypoint" ))
			{
				uint32 type_checksum = 0;		
				p_node_struct->GetChecksum( CRCD( 0x7321a8d6, "Type" ), &type_checksum );
				if( type_checksum == CRCD( 0x9784feb5, "HorseNav" ))
				{
					AddNavNode( i, p_node_struct );
					++m_current_node;
				}
			}
		}
	}
	
	// We are creating a table of all nodes, and the pointer to the CNavNode for that node, so we can do a reverse lookup.
	int num_nodes = p_nodearray->GetSize();
	CNavNode **pp_navnodes = (CNavNode**)Mem::Malloc( num_nodes * sizeof( CNavNode* ));
	for( int i = 0; i < num_nodes; ++i )
	{
		pp_navnodes[i] = NULL;
	}

	// Now fill it in.
	CNavNode* p_node;
	for( int node = 0; node < m_num_nodes; ++node )
	{
		p_node = &mp_nodes[node];
		pp_navnodes[p_node->GetNode()] = p_node;
	}

	// Now go through all the nodes.
	for( int node = 0; node < m_num_nodes; ++node )
	{
		p_node = &mp_nodes[node];
		for( int i = 0; i < p_node->m_num_links; ++i )
		{
			int index = (int)p_node->mp_links[i];

			Dbg_MsgAssert( index < num_nodes, ( "Node %d, Nav link node (%d) out of range (0 .. %d). Bad Node array?", p_node->m_node, index, num_nodes ));
			Dbg_MsgAssert( pp_navnodes[index], ( "NavNode %d linked to something (node %d) that is not a NavNode", p_node->m_node, index ));

			p_node->mp_links[i] = pp_navnodes[index];
		}
	}

	// Don't need the temporary array anymore.
	Mem::Free( pp_navnodes );
}



Mth::Vector	CNavManager::GetPos( const CNavNode* p_node ) const
{
	Dbg_Assert( p_node );
	return p_node->GetPos();
}


}
			   

