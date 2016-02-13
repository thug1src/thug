//////////////////////////////////////////
// rail.h

#ifndef __OBJECTS_RAIL_H
#define __OBJECTS_RAIL_H
                        
#include <core/defines.h>
#include <core/math.h>
#include <gfx/nxflags.h>
                           
namespace Script
{
	class	CArray;
	class	CStruct;
}

#define	MAX_RAIL_LINKS 4


namespace Obj
{

class	CRailManager;
class	CWalkComponent;
struct	SHangRailData;
struct	SLadderRailData;

enum ERailNodeFlag
{	
	LIP_OVERRIDE,		// always do a lip trick on this
	DEFAULT_LINE,
	ACTIVE,
	NO_CLIMBING,
	ONLY_CLIMBING,
	LADDER,
	NO_HANG_WITH_RIGHT_ALONG_RAIL,
	NO_HANG_WITH_LEFT_ALONG_RAIL
};


// seperating out a parallel structure of CRailLinks, as only used 
// when rails are first parsed
class	CRailLinks
{
	friend class CRailManager;
	sint16 m_link[MAX_RAIL_LINKS];	// link numbers	
};

class CRailNode : public Spt::Class
{
	friend class  CRailManager;

public:						  
	bool 			ProbablyOnSameRailAs(int SearchNode) const;
	int				Side(const Mth::Vector &vel) const;
	
	const CRailNode	*	GetNextLink() const				{return m_pNextLink;}	
	const CRailNode	*	GetPrevLink() const				{return m_pPrevLink;}	
	const sint16			GetNode() const 	  				{return m_node;}
//	const sint16			GetLink() const	  				{return m_link;}
	const ETerrainType	GetTerrain() const	  			{return (ETerrainType) m_terrain_type;}
	
	void				SetActive(bool active)		{if (active) SetFlag(ACTIVE); else ClearFlag(ACTIVE);}
	const bool			GetActive() const					{return GetFlag(ACTIVE);}
	const bool			IsActive() const					{return GetActive();}

	void			SetFlag(ERailNodeFlag flag) {m_flags.Set(flag);}
	void			ClearFlag(ERailNodeFlag flag) {m_flags.Clear(flag);}
	bool			GetFlag(ERailNodeFlag flag) const {return m_flags.Test(flag);}

protected:

// MEMOPT:  In theory this could all fit in 3x16 = 48 bytes, instead it is 64, becaute the Mth::Vectors are 16 byte aligned
// possibly could define a Mth::Vector3 that uses 12 bytes, with appropiate copy operators.

	Mth::Vector m_BBMin;		// bounding box for this node and the subsequent one
	Mth::Vector m_BBMax;		// (or just a zero-sized box, if not applicable)
	Mth::Vector m_pos;			// position of the node

    Mth::Quat m_orientation;    // orientation of the rail node; only set for climbing nodes
	
	CRailNode *m_pNextLink;		// Pointer to next Node	in rail(or NULL if none)
	CRailNode *m_pPrevLink;		// Pointer to previous Node	(or NULL if none)

	Flags< ERailNodeFlag > m_flags; // flags for rail segment
	
	sint16 m_node;				// Number of the node in the trigger array	
	uint8 m_terrain_type;		// play the correct grind sound on the rail...
	
    

// the position getting functions are private
// so you can only access them through the rail manager
// (as you need to know if it's transformed or not)
private:
	const Mth::Vector&	GetPos() const;	 
	const Mth::Quat&	GetOrientation() const;	 
	const Mth::Vector&	GetBBMin() const;
	const Mth::Vector&	GetBBMax() const;
						  
};



class CRailManager : public Spt::Class
{

public:
					CRailManager();
				   ~CRailManager();	

	
	void 			Cleanup();
								 
	void			AddRailsFromNodeArray(Script::CArray * p_node_array);
	void 			AddRailNode(int node_number, Script::CStruct *p_node_struct);
	void 			AddedAll();
	
	void			UpdateTransform(Mth::Matrix& transform);
    bool			CheckForLadderRail ( const Mth::Vector& pos, float max_horizontal_snap_distance, float max_vertical_snap_distance, bool up, CWalkComponent* p_walk_component, SLadderRailData& rail_data, const CRailNode** pp_rail_node );
	bool			CheckForAirGrabLadderRail ( const Mth::Vector& start_pos, const Mth::Vector& end_pos, CWalkComponent* p_walk_component, SLadderRailData& rail_data, const CRailNode** pp_rail_node );
	// bool			CheckForHopToHangRail ( const Mth::Vector& check_line_start, float check_line_height, const Mth::Vector& facing, float max_forward_reach, float max_backward_reach, CWalkComponent* p_walk_component, SHangRailData& rail_data, const CRailNode** pp_rail_node );
	bool			CheckForHangRail ( const Mth::Vector& start_pos, const Mth::Vector& end_pos, const Mth::Vector& facing, CWalkComponent* p_walk_component, SHangRailData& rail_data, float snap_distance );
	bool			RailNodesAreCoincident ( const CRailNode* p_node_a, const CRailNode* p_node_b );
	bool			CheckForCoincidentRailNode ( const CRailNode* p_node, uint32 ignore_mask, const CRailNode** pp_next_node );
    bool 			StickToRail(const Mth::Vector &pos1, const Mth::Vector &pos2, Mth::Vector *p_point, Obj::CRailNode **pp_rail_node, const Obj::CRailNode *p_ignore_node = NULL, float min_dot = 1.0f, int side = 0);
	
	void 			SetActive( int node, int active, bool wholeRail );
	bool 			IsActive( int node );
	bool			IsMoving(){return m_is_transformed;}

	void 			MoveNode( int node, Mth::Vector &pos );

	
	void			DebugRender(Mth::Matrix *p_transform = NULL);
	void			RemoveOverlapping();
	
	int				GetNumNodes(){return m_num_nodes;}
	
	Script::CArray *	GetNodeArray(){return mp_node_array;}
	void				SetNodeArray(Script::CArray *p_nodeArray){mp_node_array = p_nodeArray;}

	void			NewLink(CRailNode *p_from, CRailNode *p_to);

	void			AutoGenerateRails(Script::CStruct* pParams);

	CRailNode*		GetRailNodeByNodeNumber( int node_num );
	
	int				GetNodeIndex(const CRailNode *p_node) {return (((int)(p_node)-(int(mp_nodes)))/sizeof(CRailNode));}
	
// accessor functions for the rail node
// we only access their positions via the rail manager
// as only the rail manager knows the transform of the object to
// which they are attached
// These nodes are "instances", in that the same node might be used in
// more than one place
// so these functions only make sense in conjunction with the rail_manager
	Mth::Vector LocalToWorldTransform ( const Mth::Vector& vector ) const;
	Mth::Vector	GetPos(const CRailNode *p_node) 	const;	 
    Mth::Matrix GetMatrix(const CRailNode *p_node) 	const;
	Mth::Vector	GetBBMin(const CRailNode *p_node)   const;
	Mth::Vector	GetBBMax(const CRailNode *p_node)   const;


private:
//	CRailNode*  		mp_first_node;			// was a pointer to a list of nodes
	Mth::Matrix			m_transform;
	CRailNode*  		mp_nodes;				// pointer to array of nodes
	CRailLinks*			mp_links;				// pointer to temp array of links
	Script::CArray *	mp_node_array;
	int					m_num_nodes;
	int					m_current_node;
	bool				m_is_transformed;
	
};



// Global rail type functions that don't need a manager

bool  			Rail_ValidInEditor(Mth::Vector Start, Mth::Vector End);
 
inline const Mth::Vector&	CRailNode::GetPos() const
{
	return m_pos;
}

inline const Mth::Quat&	CRailNode::GetOrientation() const
{
	return m_orientation;
}

inline const Mth::Vector&	CRailNode::GetBBMin() const
{
	return m_BBMin;
}

inline const Mth::Vector&	CRailNode::GetBBMax()  const
{
	return m_BBMax;
}

} 

			
#endif			// #ifndef __OBJECTS_RAIL_H
			


