//////////////////////////////////////////
// navigation.h

#ifndef __OBJECTS_NAVIGATION_H
#define __OBJECTS_NAVIGATION_H
                        
#include <core/defines.h>
#include <core/math.h>
#include <gfx/nxflags.h>
                           
namespace Script
{
	class	CArray;
	class	CStruct;
}

#define	MAX_NAV_LINKS 12

namespace Obj
{

class	CNavManager;
//class	CWalkComponent;
//struct	SHangNavData;
//struct	SLadderNavData;

//enum ENavNodeFlag
//{	
//	LIP_OVERRIDE,		// always do a lip trick on this
//	DEFAULT_LINE,
//	ACTIVE,
//	CLIMBING,
//	LADDER
//};



// Forward declaration.
class CNavNode;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sAStarNode
{
	enum EAStarNodeFlag
	{
		IN_OPEN		= 0x01,
		IN_CLOSED	= 0x02
	};

	uint32			m_flags;
	float			m_cost_from_start;
	float			m_cost_to_goal; 
	float			m_total_cost; 
	CNavNode*		mp_parent;


	bool			InOpen( void )		{ return m_flags & IN_OPEN; }
	bool			InClosed( void )	{ return m_flags & IN_CLOSED; }
	void			Reset();
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CNavNode : public Spt::Class
{
	friend class		CNavManager;

public:						  
	
	const sint16		GetNode() const				{ return m_node; }
	sAStarNode*			GetAStarNode( void )		{ return &m_astar_node; }
	Mth::Vector&		GetPos( void )				{ return m_pos; }
	int					GetNumLinks( void )			{ return m_num_links; }
	CNavNode*			GetLink( int l )			{ return mp_links[l]; }

protected:

	Mth::Vector			m_pos;						// Position of the node.
	CNavNode*			mp_links[MAX_NAV_LINKS];
	sint16				m_num_links;
	sint16				m_node;						// Number of the node in the trigger array.
	sAStarNode			m_astar_node;				// Used during the A* pathfinding routine.

private:
	const Mth::Vector&	GetPos() const;
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CNavManager : public Spt::Class
{

public:
						CNavManager();
						~CNavManager();	
	
	void 				Cleanup();
									 
	void				AddNavsFromNodeArray( Script::CArray * p_node_array );
	void 				AddNavNode( int node_number, Script::CStruct *p_node_struct );
	
	int					GetNumNodes()									{ return m_num_nodes; }
	Script::CArray*		GetNodeArray()									{ return mp_node_array; }
	void				SetNodeArray( Script::CArray* p_nodeArray )		{ mp_node_array = p_nodeArray; }

	CNavNode*			GetNavNodeByIndex( int index )					{ return mp_nodes + index; }
	CNavNode*			GetNavNodeByNodeNumber( int node_num );
	
	Mth::Vector			GetPos( const CNavNode *p_node ) const;	 

private:

	CNavNode*	  		mp_nodes;				// pointer to array of nodes
	Script::CArray*		mp_node_array;
	int					m_num_nodes;
	int					m_current_node;
};
 


CNavNode* CalculateNodePath( Mth::Vector pos, Mth::Vector target_pos );


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline const Mth::Vector& CNavNode::GetPos() const
{
	return m_pos;
}





} 

			
#endif			// #ifndef __OBJECTS_RAIL_H
			


