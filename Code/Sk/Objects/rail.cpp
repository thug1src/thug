///////////////////////////////////////////////////////
// rail.cpp

#include <sk/objects/rail.h>

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
#include <gel/components/walkcomponent.h>

#include <gfx/nx.h>
#include <gfx/debuggfx.h>

#include <sk/engine/feeler.h>
#include <sk/objects/trickobject.h>
#include <sk/modules/skate/skate.h>
#include <sk/ParkEditor2/ParkEd.h>
#include <sk/scripting/nodearray.h>
		  
// REMOVE
#include <sk/objects/skater.h>
		  
#ifdef	__PLAT_NGPS__	
#include <gfx/nxviewman.h>			// for camera
#include <gfx/ngps/nx/line.h>
#include <gfx/ngps/nx/render.h>
#endif

//#define		DEBUG_EDITOR_RAILS

namespace Obj
{


void Rail_ComputeBB(Mth::Vector &pos1, Mth::Vector &pos2, Mth::Vector &bb_min, Mth::Vector &bb_max)
{
	float x0 = pos1.GetX();
	float x1 = pos2.GetX();
	float min_x, max_x;
	if (x0 < x1)
	{
		min_x = x0; max_x = x1;
	}
	else
	{
		min_x = x1; max_x = x0;
	}

	float y0 = pos1.GetY();
	float y1 = pos2.GetY();
	float min_y, max_y;
	if (y0 < y1)
	{
		min_y = y0; max_y = y1;
	}
	else
	{
		min_y = y1; max_y = y0;
	}

	float z0 = pos1.GetZ();
	float z1 = pos2.GetZ();
	float min_z, max_z;
	if (z0 < z1)
	{
		min_z = z0; max_z = z1;
	}
	else
	{
		min_z = z1; max_z = z0;
	}

	bb_min.Set(min_x, min_y, min_z);
	bb_max.Set(max_x, max_y, max_z);
}

CRailManager::CRailManager()
{
//	mp_first_node = NULL;
	mp_nodes = NULL;
	mp_links = NULL;
	m_num_nodes = 0;
	m_is_transformed = false;
	mp_node_array = NULL;
	
}

CRailManager::~CRailManager()
{	 
	Cleanup();  
}

void CRailManager::Cleanup()
{
//	while (mp_first_node)
//	{
//		CRailNode *pNext = mp_first_node->m_pNext;
//		delete mp_first_node;
//		mp_first_node = pNext;			
//	}    
	
	m_num_nodes = 0;
	
	Mem::Free(mp_nodes);
	mp_nodes = NULL;
	
	
	m_is_transformed = false;	
	// Unhook from node array
	mp_node_array = NULL;
}


#define CHECKSUM_TERRAIN_TYPE	0x54cf8532  // "terrainType"

// Given two rail nodes that should be linked in a rail segment
// then link them up as well as we are able
// given that we only have a NEXT and a PREV pointer
// so we give precedence to links to nodes that
// are flagged "DefaultLine"

void CRailManager::NewLink(CRailNode *p_from, CRailNode *p_to)
{
	// one of our links links to this node
	// so we need to establish next and prev links
	// or if they are established, then possibly override them			
	if (!p_from->m_pNextLink)
	{
		// this is the first time we are linking to something
		p_from->m_pNextLink = p_to;
		Rail_ComputeBB(p_from->m_pos, p_to->m_pos, p_from->m_BBMin, p_from->m_BBMax);
	}
	else
	{
	
		// already linked to something, so only override it
		// if the new node is not DEFAULT_LINE
		if (p_to->GetFlag(DEFAULT_LINE))
		{
			// This new link is DEFAULT_LINE
			// so, as long as the previous link was not, we can override it
			Dbg_MsgAssert(!p_from->m_pNextLink->GetFlag(DEFAULT_LINE),
						  ("Node %d links to both %d and %d and both are DEFAULT_LINE",
						   p_from->m_node,p_from->m_pNextLink->m_node,p_to->m_node));
			// Overriding an existing NEXT link
			p_from->m_pNextLink = p_to;
			Rail_ComputeBB(p_from->m_pos, p_to->m_pos, p_from->m_BBMin, p_from->m_BBMax);
		}
		else
		{
			// The new link is not DEFAULT_LINE
			// if the old one was, then leave it
			// if neither is, then leave it, and maybe print a warning?
			
		}
	}
	
	// Now handle the PREV link back from p_to to p_from
	
	if (!p_to->m_pPrevLink)
	{
		p_to->m_pPrevLink = p_from;
	}
	else
	{

		// something already linked to p_from via prev from p_to
		if (p_from->GetFlag(DEFAULT_LINE))
		{
			// this is the default line, so make sure the old PREV node was not
			Dbg_MsgAssert(!p_to->m_pPrevLink->GetFlag(DEFAULT_LINE),
						  ("Node %d linked from both %d and %d and both are DEFAULT_LINE",
						   p_to->m_node,p_to->m_pPrevLink->m_node,p_from->m_node));
			
			p_to->m_pPrevLink = p_from;
			// Note, we don't re-calculate the bounding box here
			// as that only applies to NEXT links
		}
	}
}


void CRailManager::AddRailNode(int node_number, Script::CStruct *p_node_struct)
{

							
	// create a new rail node										   
//	CRailNode *pRailNode = new CRailNode();
	// Link into the head of the rail manager	
	// Note, this must be done before we attempt to link up with other nodes								   
//	pRailNode->m_pNext = mp_first_node; 							  
//	mp_first_node = pRailNode;
	
	CRailNode *pRailNode = &mp_nodes[m_current_node];
	CRailLinks *pLinkNode = &mp_links[m_current_node++];
	
	uint32 class_checksum;
	p_node_struct->GetChecksum(CRCD(0x12b4e660, "class"), &class_checksum);
	bool climbing = class_checksum == CRCD(0x30c19600, "ClimbingNode");
	
	uint32 type_checksum;
	p_node_struct->GetChecksum(CRCD(0x7321a8d6, "type"), &type_checksum);

	pRailNode->m_pNextLink = NULL;
	pRailNode->m_pPrevLink = NULL;
	for (int i=0;i<MAX_RAIL_LINKS;i++)
	{
		pLinkNode->m_link[i] = -1;						// say it's not linked to anything, initially
	}
	pRailNode->m_node = node_number;			// The node_number is use primarily for calculating links
	pRailNode->m_flags = 0;						// all flags off by default
	pRailNode->SetActive(p_node_struct->ContainsFlag( 0x7c2552b9 /*"CreatedAtStart"*/ ));	// created at start or not?
	
    // set the position from the node structure
	SkateScript::GetPosition(p_node_struct,&pRailNode->m_pos);

	if (p_node_struct->ContainsFlag( 0xf4c67f0f /*"LipOverride"*/))
	{
		pRailNode->m_flags.Set(LIP_OVERRIDE);
	}

	if (p_node_struct->ContainsFlag(  0x4de721de/*"DefaultLine"*/))
	{
		pRailNode->m_flags.Set(DEFAULT_LINE);
	}
	
	if (climbing)
	{
		pRailNode->m_flags.Set(ONLY_CLIMBING);
		pRailNode->m_flags.Set(LADDER, type_checksum == CRCD(0xc84243da, "Ladder"));
	}
	
	pRailNode->m_flags.Set(NO_HANG_WITH_RIGHT_ALONG_RAIL, p_node_struct->ContainsFlag(CRCD(0xf6dd21ca, "HangLeft")));
	pRailNode->m_flags.Set(NO_HANG_WITH_LEFT_ALONG_RAIL, p_node_struct->ContainsFlag(CRCD(0x5e9ce29b, "HangRight")));
	pRailNode->m_flags.Set(NO_CLIMBING, p_node_struct->ContainsFlag(CRCD(0xf22bfc6d, "NoClimbing")));
    
	if (climbing)
	{
		// set the orientation from the node structure
		Mth::Vector angles;
		SkateScript::GetAngles(p_node_struct, &angles);
		Mth::Matrix matrix;
		pRailNode->m_orientation = matrix.SetFromAngles(angles);
	}

	// no terrain types for climbing nodes
	if (!climbing)
	{
		pRailNode->m_terrain_type = 0;  // default terrain type...
		uint32 terrain_checksum;
		int terrain = 0;
		p_node_struct->GetChecksum( CHECKSUM_TERRAIN_TYPE, &terrain_checksum, Script::ASSERT );
		terrain = Env::CTerrainManager::sGetTerrainFromChecksum(terrain_checksum);
		Dbg_MsgAssert(terrain >= 0 && terrain < 256,("Rail node %d, terrain type %d too big\n",node_number,terrain));
		pRailNode->m_terrain_type = (uint8) terrain ;
	}


	int links = SkateScript::GetNumLinks(p_node_struct);
	if (links)
	{
		Dbg_MsgAssert(links <= MAX_RAIL_LINKS,("Rail node %d has %d linkes, max is %d",node_number,links,MAX_RAIL_LINKS));
		for (int i=0;i<links;i++)
		{
			pLinkNode->m_link[i] = SkateScript::GetLink(p_node_struct,i);
		}
	}	
	
	Rail_ComputeBB(pRailNode->m_pos, pRailNode->m_pos, pRailNode->m_BBMin, pRailNode->m_BBMax);

// Linking is now donw after they are all loaded
// Making it O(n) rather than O(n^2)
/*	
	// try going through all the nodes to find one linked to this one
	// and to find the target node, if it exists
	CRailNode *p_link_node = pRailNode->m_pNext;
	while (p_link_node)
	{
		// Check links from the new node to all the existing nodes
		for (int i=0;i<MAX_RAIL_LINKS;i++)
		{
			if (pRailNode->m_link[i] == p_link_node->m_node)
			{
				NewLink(pRailNode,p_link_node);
			}
		}
		
		// check links from the existing nodes to the new node													  
		for (int i=0;i<MAX_RAIL_LINKS;i++)
		{
			if (p_link_node->m_link[i] == pRailNode->m_node)
			{
				NewLink(p_link_node,pRailNode);
			}
		}
	   	p_link_node = p_link_node->m_pNext;		
	}
*/	
}

// returns a positive number that is the amount of an axis
// required to totally encompass the 1D line segments a-b and c-d
static inline float span_4(float a, float b, float c, float d)
{
	float min = a;
	if (b < min) min = b;
	if (c < min) min = c;
	if (d < min) min = d;
	float max = a;
	if (b > max) max = b;
	if (c > max) max = c;
	if (d > max) max = d;
	return max-min;	
}

															 
// returns the amount two 1D line segments overlap
// works by adding together the length of both lines
// and then subtracting the "span" of the the min and max points of the lines															 
static inline float overlap(float a, float b, float c, float d)
{
	return  Mth::Abs(b-a) + Mth::Abs(d-c) - span_4(a,b,c,d);
}

static inline bool nearly_the_same(float a, float b, float c, float d, float nearly = 1.0f)
{
	return ( Mth::Abs(a-b) < nearly
		 && Mth::Abs(b-c) < nearly
		 && Mth::Abs(c-d) < nearly);
}	

void	CRailManager::RemoveOverlapping()
{
	printf ("Starting overlapping rail removal\n");
	
	int	removed =0;
//	CRailNode *pRailNode = mp_first_node;
//	while (pRailNode)
//	{
	for (int node=0;node<m_num_nodes;node++)
	{
		CRailNode *pRailNode = &mp_nodes[node];
		if (pRailNode->m_pNextLink)	   	// it's a segment
		{

			Mth::Vector	start = pRailNode->m_pos;
			Mth::Vector	end = pRailNode->m_pNextLink->m_pos;

			Mth::Vector dir = end-start;
			dir.Normalize();

			// we expand the bounding box, as they might not really be that close			
			Mth::Vector bb_min = pRailNode->m_BBMin;
			bb_min[X] -= 16.0f;
			bb_min[Y] -= 16.0f;
			bb_min[Z] -= 16.0f;
			Mth::Vector bb_max = pRailNode->m_BBMax;
			bb_max[X] += 16.0f;
			bb_max[Y] += 16.0f;
			bb_max[Z] += 16.0f;
			
			
			int check_node = node+1;
//			CRailNode *pCheckNode = pRailNode->m_pNext;
			while (check_node<m_num_nodes && pRailNode->IsActive())
			{
				CRailNode *pCheckNode = &mp_nodes[check_node];
				if (pCheckNode->m_pNextLink)	   	// it's a segment
				{
				   	// first check to see if bounding boxes overlap
					  
					if (!(pCheckNode->m_BBMin.GetX() > bb_max.GetX()))
					if (!(pCheckNode->m_BBMax.GetX() < bb_min.GetX()))
					if (!(pCheckNode->m_BBMin.GetZ() > bb_max.GetZ()))
					if (!(pCheckNode->m_BBMax.GetZ() < bb_min.GetZ()))
					if (!(pCheckNode->m_BBMin.GetY() > bb_max.GetY()))
					if (!(pCheckNode->m_BBMax.GetY() < bb_min.GetY()))
					{					
					
						// bounding boxes overlap 						
						// check to see if rails are roughly parallel
						Mth::Vector check_start = pCheckNode->m_pos;
						Mth::Vector check_end = pCheckNode->m_pNextLink->m_pos;
						Mth::Vector check_dir = check_end - check_start;

						check_dir.Normalize();						
						float dot = Mth::DotProduct(dir, check_dir);
//						printf ("Bounding Box Overlap, dot = %f\n",dot);
						
						if (dot < -0.99f || dot > 0.99f)
						{
							// lines are roughly parallel
							// so check if they overlap significantly in at least one axis
							// and/or are very close in one other axis
							// we only check X and Z
							// as we don't have any vertical rails in the park
							// editor
							// (note, I'm prematurely optimizing here...)
							
							int overlaps = 0;
							int close =  0;
							
							// now check the distance between the lines
							// which is the components of start -> check_start
							// that is perpendicular to start -> end
							const float significant = 6.0f;	// six inches is significant, I feel...
							Mth::Vector	s_cs = check_start - start;
							Mth::Vector	s_e = end - start;
							s_e.Normalize();
							Mth::Vector perp = s_cs;
							perp.ProjectToPlane(s_e); 
							if ( perp.Length()<significant)
							{
	
//								printf ("(%.2f,%.2f),(%.2f,%.2f),(%.2f,%.2f),(%.2f,%.2f)\n",
//								start[X],start[Z],end[X],end[Z],
//								check_start[X],check_start[Z],check_end[X],check_end[Z]);
								
								if (overlap(start[X],end[X],check_start[X],check_end[X]) > significant)
									overlaps++;
								if (overlap(start[Z],end[Z],check_start[Z],check_end[Z]) > significant)
									overlaps++;
								if (nearly_the_same(start[X],end[X],check_start[X],check_end[X],6.0f))
									close++;
								if (nearly_the_same(start[Z],end[Z],check_start[Z],check_end[Z],6.0f))
									close++;
//								printf ("dot close, overlaps = %d, close = %d\n",overlaps,close);
								if (overlaps + close >= 2)
								{
									// it's a duplicate
//									printf("Removing duplicate rail !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
									
									// add a magenta line for rails that have been removed
									#ifdef		DEBUG_EDITOR_RAILS
									pRailNode->m_pos += Mth::Vector(0.0f,2.0f,0.0f);
									pRailNode->m_pNextLink->m_pos += Mth::Vector(0.0f,2.0f,0.0f);
									Gfx::AddDebugLine( pRailNode->m_pos, pRailNode->m_pNextLink->m_pos, MAKE_RGB( 255, 0, 255 ),MAKE_RGB( 255,0, 255 ) );
									pRailNode->m_pos -= Mth::Vector(0.0f,2.0f,0.0f);
									pRailNode->m_pNextLink->m_pos -= Mth::Vector(0.0f,2.0f,0.0f);
									#endif
									
									removed ++;
									// Remove the one that is lower
									if (pRailNode->m_pos[Y] < pCheckNode->m_pos[Y])
									{
										pRailNode->SetActive(false);
									}
									else
									{
										pCheckNode->SetActive(false);
									}
									break;
								}
							}
						}
					}
				}
				check_node++;
//				pCheckNode = pCheckNode->m_pNext;
			}				
		}
//		pRailNode = pRailNode->m_pNext;
	}		 
	printf ("Done overlapping rail removal, removed %d\n", removed);
}


void CRailManager::AddedAll()
{
	
//	printf("Added all rails\n");		 

	#if 1
	if (Ed::CParkEditor::Instance()->UsingCustomPark()) 		// is it a custom park???
	{
		// yes, so remove overlapping rails
		RemoveOverlapping();	
	}	 
	#endif

	
}			   

void CRailManager::SetActive( int node, int active, bool wholeRail )
{
	
//	CRailNode *pRailNode = mp_first_node;
//	while (pRailNode)
	for (int check_node = 0;check_node<m_num_nodes;check_node++)
	{
		CRailNode *pRailNode = &mp_nodes[check_node];
		
		if (pRailNode->m_node == node)
		{
			if ( wholeRail )
			{
				while ( pRailNode )
				{
					pRailNode->SetActive(active);
					pRailNode = pRailNode->m_pNextLink;
				}
				return;
			}
			pRailNode->SetActive(active);
			return;
		}
//		pRailNode = pRailNode->m_pNext;
	}				 
}

bool CRailManager::IsActive( int node )
{
//	CRailNode *pRailNode = mp_first_node;
//	while (pRailNode)
	for (int check_node = 0;check_node<m_num_nodes;check_node++)
	{
		CRailNode *pRailNode = &mp_nodes[check_node];
		if (pRailNode->m_node == node)
		{
			return ( pRailNode->GetActive() );
		}
//		pRailNode = pRailNode->m_pNext;
	}
	return ( false );
}


// A node in the node array has moved (due to realtime editing)
// so find that node here, and update the position
// and update bounding boxes as needed 
//

void CRailManager::MoveNode( int node, Mth::Vector &pos )
{
	for (int check_node = 0;check_node<m_num_nodes;check_node++)
	{
		CRailNode *pRailNode = &mp_nodes[check_node];
		
		if (pRailNode->m_node == node)
		{
			// Set The position
			pRailNode->m_pos = pos;
			
			// calculate the bounding box of any segment this node starts
			// (there can be only one)
			if (pRailNode->GetNextLink())
			{
				Mth::Vector from_pos = pRailNode->m_pos; 
				Mth::Vector to_pos = pRailNode->GetNextLink()->m_pos;
				Rail_ComputeBB(from_pos ,to_pos , pRailNode->m_BBMin, pRailNode->m_BBMax);				
			}
			
			// Check to see if there are any nodes leading to this one
			// (this initial check is only for speed)
			if (pRailNode->GetPrevLink())
			{
				// There is one, but there might be more that one
				// the only way to know, is to go through all the nodes and see if any points to this one
				// 
				for (int from_node = 0;from_node<m_num_nodes;from_node++)
				{
					CRailNode *p_from = &mp_nodes[check_node];
					if (p_from->GetNextLink() == pRailNode)
					{
						Rail_ComputeBB(p_from->m_pos, pRailNode->m_pos, p_from->m_BBMin, p_from->m_BBMax);
					}
				}
			}
			// There is only going to be one mathcing node, so we can now return		
			return;
		} // end if (pRailNode->m_node == node)
	}				 
}




// Given a velocity, and a rail node
// then see which side of the rail (left or right)
// has no collision
// returns -1 if left, 0 if neither or both, and +1 if right
// Used in the park editor to give preference to those
// rails that keep the same sideness as the prior rail we were on
int	CRailNode::Side(const Mth::Vector &vel) const
{
	Mth::Vector Start = m_pos;
	Mth::Vector End = 	m_pNextLink->m_pos;

	CFeeler	feeler;

 // Find a point "Mid", which is 1/4 of the way along the rail   
 // and lowered six inches
	Mth::Vector	Mid = (Start + End) / 2.0f;			// Half way along
	Mid = Start + ((Mid - Start) / 2.0f);		    // quarter of the way along	
	Mid[Y] -= 6.0f;									// lowered a bit

// Find a vector "Side" which is horizontal, perpendicular to the rail, and 4 inches long	
	Mth::Vector	Side = End - Start;	   			// vector along the rail
	Side[Y] = 0.0f;								// horizontal
	Side.Normalize(4.0f);						// this is half the width of the thickest rail.  Was 4.0, changed for Tokyo mega funbox
	float temp = Side[X];			 				// make perpendicular to rail
	Side[X] = Side[Z];
	Side[Z] = -temp;


	bool	left = false;
	bool	right = false;
	
// if collision above me, left to right
	feeler.SetLine(Mid + Side, Mid-Side);	
	if (feeler.GetCollision())
	{
		left = true;
	}

// if collision above me, right to left
	feeler.FlipDirection();
	if (feeler.GetCollision())
	{
		right = true;
	}
	else
	{
	}
	
	int side = 0;
	if (left && !right)
	{
		side = -1;
	}
	else
	{
		if (right && ! left)
		{
			side = 1;
		}
	}
	
	// flip is we are going the opposite direction on the rail
	if ( side && (Mth::DotProduct(vel,End-Start) < 0.0f) )
	{
		side = -side;
	}
	
	return side;	
}


void CRailManager::UpdateTransform(Mth::Matrix& transform)
{

	m_is_transformed = true;
	m_transform = transform;
}


bool CRailManager::CheckForLadderRail ( const Mth::Vector& pos, float max_horizontal_snap_distance, float max_vertical_snap_distance, bool up, CWalkComponent* p_walk_component, SLadderRailData& rail_data, const CRailNode** pp_rail_node )
{
    float max_horizontal_snap_distance_sqr = Mth::Sqr(max_horizontal_snap_distance);
    
    for (int check_node_idx = m_num_nodes; check_node_idx--; )
    {
        CRailNode* p_node = &mp_nodes[check_node_idx];
        
		if (!p_node->GetFlag(LADDER) || !p_node->GetActive()) continue;
        
        // quick oriented bounding box check
        if (Mth::Abs(p_node->m_pos[X] - pos[X]) > max_horizontal_snap_distance) continue;
        if (Mth::Abs(p_node->m_pos[Z] - pos[Z]) > max_horizontal_snap_distance) continue;
        if (Mth::Abs(p_node->m_pos[Y] - pos[Y]) > max_vertical_snap_distance) continue;
        
        // actual distance check
        Mth::Vector offset = p_node->m_pos - pos;
        if (offset[X] * offset[X] + offset[Z] * offset[Z] > max_horizontal_snap_distance_sqr) continue;
        
        // find the ladder's partiner node
        const CRailNode* p_partiner_node = p_node->GetNextLink() ? p_node->GetNextLink() : p_node->GetPrevLink();
        Dbg_Assert(p_partiner_node);
        
		if (up)
		{
			if (!p_walk_component->FilterLadderUpRail(p_node->m_pos, p_partiner_node->m_pos, GetMatrix(p_node), rail_data)) continue;
		}
		else
		{
			if (!p_walk_component->FilterLadderDownRail(p_partiner_node->m_pos, p_node->m_pos, GetMatrix(p_partiner_node), rail_data)) continue;
		}
        
        // for now we do no comparison between ladders; they should be sparce enough that we can just take the first ladder rail we find
		
		// return the bottom node of the ladder
		if (up)
		{
			*pp_rail_node = p_node;
		}
		else
		{
			*pp_rail_node = p_partiner_node;
		}
        return true;
    }
    
    return false;
}

bool CRailManager::CheckForAirGrabLadderRail ( const Mth::Vector& start_pos, const Mth::Vector& end_pos, CWalkComponent* p_walk_component, SLadderRailData& rail_data, const CRailNode** pp_rail_node )
{
	*pp_rail_node = NULL;
	
	Mth::Line movement;
	movement.m_start = start_pos;
	movement.m_end = end_pos;
	
	// find bounding box for movement
	Mth::Vector bb_min, bb_max;
	Rail_ComputeBB(movement.m_start, movement.m_end, bb_min, bb_max);
	float snap_dist_sqr = Script::GetFloat(CRCD(0x30ce7f2c, "Climb_Max_Snap"));
	bb_min.Set(bb_min.GetX() - snap_dist_sqr, bb_min.GetY() - snap_dist_sqr, bb_min.GetZ() - snap_dist_sqr);	
	bb_max.Set(bb_max.GetX() + snap_dist_sqr, bb_max.GetY() + snap_dist_sqr, bb_max.GetZ() + snap_dist_sqr);	
	snap_dist_sqr *= snap_dist_sqr;
	
	float closest_dist_sqr = 10000000.0f * 10000000.0f;

	for (int check_node = 0; check_node < m_num_nodes; check_node++)
	{
		CRailNode* pRailNode = &mp_nodes[check_node];
		if (!pRailNode->GetFlag(LADDER) || !pRailNode->GetActive() || !pRailNode->m_pNextLink) continue;
		
		// First do bounding box test, before time-intensive LineLineIntersect test 
		if (bb_min.GetX() > pRailNode->m_BBMax.GetX()) continue;
		if (bb_max.GetX() < pRailNode->m_BBMin.GetX()) continue;
		if (bb_min.GetZ() > pRailNode->m_BBMax.GetZ()) continue;
		if (bb_max.GetZ() < pRailNode->m_BBMin.GetZ()) continue;
		if (bb_min.GetY() > pRailNode->m_BBMax.GetY()) continue;
		if (bb_max.GetY() < pRailNode->m_BBMin.GetY()) continue;
		
		// we have a rail segment with a BB match
		// so see if we are close to it 
		Mth::Line segment;
		segment.m_start = pRailNode->m_pos;
		segment.m_end = pRailNode->m_pNextLink->m_pos;
		Mth::Vector p1, p2;
		float f1,f2;
		if (!Mth::LineLineIntersect(movement, segment, &p1, &p2, &f1, &f2)) continue;
		
		Mth::Vector	to_rail = p2 - p1;
		float dist_sqr = to_rail.LengthSqr();
		
		if (dist_sqr > snap_dist_sqr || dist_sqr > closest_dist_sqr) continue;
		
		SLadderRailData this_data;
		if (pRailNode->m_pos[Y] < pRailNode->m_pNextLink->m_pos[Y])
		{
			if (!p_walk_component->FilterLadderAirGrabRail(pRailNode->m_pos, pRailNode->m_pNextLink->m_pos, GetMatrix(pRailNode), p2, f2, this_data)) continue;
			*pp_rail_node = pRailNode;
		}
		else
		{
			if (!p_walk_component->FilterLadderAirGrabRail(pRailNode->m_pNextLink->m_pos, pRailNode->m_pos, GetMatrix(pRailNode->m_pNextLink), p2, 1.0f - f2, this_data)) continue;
			*pp_rail_node = pRailNode->m_pNextLink;
		}
		
		closest_dist_sqr = dist_sqr;
		rail_data = this_data;
	}

	return *pp_rail_node;
}

/*
// check for rails near but perpendicular to check_line
bool CRailManager::CheckForHopToHangRail ( const Mth::Vector& check_line_start, float check_line_height, const Mth::Vector& facing, float max_forward_reach, float max_backward_reach, CWalkComponent* p_walk_component, SHangRailData& rail_data, const CRailNode** pp_rail_node )
{
	// this logic assumes Y is up, which might not be true if we're checking a movable rail manager
	// for now, no hopping to hang from movable rail managers which are not nicely oriented
	if (Mth::Abs(facing[Y]) > 0.1f) return false;
	
	// The check box is aligned with the Y axis and the facing, is check_line_height tall,
	// and has a max_foward_reach + max_backward_reach by vCHECK_BOX_WIDTH foot print in the horizontal plane.
	
	*pp_rail_node = NULL;
	float best_facing_distance = 0.0f;
	float best_height = 0.0f;
	
	float check_line_X_start = check_line_start[X] - max_backward_reach * facing[X];
	float check_line_X_delta = (max_backward_reach + max_forward_reach) * facing[X];
	float check_line_Z_start = check_line_start[Z] - max_backward_reach * facing[Z];
	float check_line_Z_delta = (max_backward_reach + max_forward_reach) * facing[Z];
	
	for (int check_node_idx = m_num_nodes; check_node_idx--; )
	{
		CRailNode* p_node = &mp_nodes[check_node_idx];
		
		if (p_node->GetFlag(LADDER) || p_node->GetFlag(NO_CLIMBING) || !p_node->GetActive() || !p_node->m_pNextLink) continue;
		
		// determine the clostest point on the rail in the horizontal plane
		float s;
		if (!Nx::CCollObj::s2DLineLineCollision(
			p_node->m_pos[X],
			p_node->m_pos[Z],
			p_node->m_pNextLink->m_pos[X] - p_node->m_pos[X],
			p_node->m_pNextLink->m_pos[Z] - p_node->m_pos[Z],
			check_line_X_start,
			check_line_Z_start,
			check_line_X_delta,
			check_line_Z_delta,
			&s
		)) continue;
		Mth::Vector closest_point = p_node->m_pos + s * (p_node->m_pNextLink->m_pos - p_node->m_pos);
		
		// calculate the offset vector from the bottom of the check line to the closest point
		Mth::Vector offset_to_rail = closest_point - check_line_start;
		
		// calculate distance along our facing to the closest point
		float facing_distance = offset_to_rail[X] * facing[X] + offset_to_rail[Z] * facing[Z];
		
		// check if the closest point is within reach
		if (facing_distance > max_forward_reach) continue;
		if (facing_distance < -max_backward_reach) continue;
		
		// check to see if the closest point is within the allowed height range
		if (closest_point[Y] < check_line_start[Y]) continue;
		if (closest_point[Y] > check_line_start[Y] + check_line_height) continue;
		
		SHangRailData this_data;
		if (!p_walk_component->FilterHangRail(GetPos(p_node), GetPos(p_node->m_pNextLink), LocalToWorldTransform(closest_point), s, this_data, false)) continue;
		
		if (*pp_rail_node)
		{
			// logic determining if this rail is better
			
			// take rail in front over rail behind
			if (best_facing_distance >= 0.0f && facing_distance < 0.0f) continue;
			if (!(facing_distance >= 0.0f && best_facing_distance < 0.0f))
			{
				// otherwise, take lowest rail
				if (offset_to_rail[Y] > best_height) continue;
				if (offset_to_rail[Y] == best_height)
				{
					// otherwise, take the closest rail horizontally
					if (Mth::Abs(facing_distance) > Mth::Abs(best_facing_distance)) continue;
				}
			}
		}
		
		// this is the best (or first) rail so far
		*pp_rail_node = p_node;
		best_facing_distance = facing_distance;
		best_height = offset_to_rail[Y];
		rail_data = this_data;
		
		Gfx::AddDebugStar(closest_point,  36.0f,  MAKE_RGB(255, 0, 0), 1);
	}
	
	return *pp_rail_node;
}
*/

bool CRailManager::CheckForHangRail ( const Mth::Vector& start_pos, const Mth::Vector& end_pos, const Mth::Vector& facing, CWalkComponent* p_walk_component, SHangRailData& rail_data, float snap_distance )
{
	bool rail_found = false;
	
	Mth::Line movement;
	movement.m_start = start_pos;
	movement.m_end = end_pos;
	
	// find bounding box for movement
	Mth::Vector bb_min, bb_max;
	Rail_ComputeBB(movement.m_start, movement.m_end, bb_min, bb_max);
	float snap_dist_sqr = snap_distance;
	bb_min.Set(bb_min.GetX() - snap_dist_sqr, bb_min.GetY() - snap_dist_sqr, bb_min.GetZ() - snap_dist_sqr);	
	bb_max.Set(bb_max.GetX() + snap_dist_sqr, bb_max.GetY() + snap_dist_sqr, bb_max.GetZ() + snap_dist_sqr);	
	snap_dist_sqr *= snap_dist_sqr;
	
	float closest_dist_sqr = 10000000.0f * 10000000.0f;

	for (int check_node = 0; check_node < m_num_nodes; check_node++)
	{
		CRailNode* pRailNode = &mp_nodes[check_node];
		if (pRailNode->GetFlag(LADDER) || pRailNode->GetFlag(NO_CLIMBING) || !pRailNode->GetActive() || !pRailNode->m_pNextLink) continue;
		
		// First do bounding box test, before time-intensive LineLineIntersect test 
		if (bb_min.GetX() > pRailNode->m_BBMax.GetX()) continue;
		if (bb_max.GetX() < pRailNode->m_BBMin.GetX()) continue;
		if (bb_min.GetZ() > pRailNode->m_BBMax.GetZ()) continue;
		if (bb_max.GetZ() < pRailNode->m_BBMin.GetZ()) continue;
		if (bb_min.GetY() > pRailNode->m_BBMax.GetY()) continue;
		if (bb_max.GetY() < pRailNode->m_BBMin.GetY()) continue;
		
		// we have a rail segment with a BB match
		// so see if we are close to it 
		Mth::Line segment;
		segment.m_start = pRailNode->m_pos;
		segment.m_end = pRailNode->m_pNextLink->m_pos;
		Mth::Vector p1, p2;
		float f1,f2;
		if (!Mth::LineLineIntersect(movement, segment, &p1, &p2, &f1, &f2)) continue;
		
		Mth::Vector	to_rail = p2 - p1;
		if (to_rail.LengthSqr() > snap_dist_sqr) continue;
		
		// count rails in front of us as three times closer
		float dot = Mth::DotProduct(facing, to_rail);
		if (dot > 0.0f)
		{
			to_rail -= (2.0f / 3.0f * dot) * facing;
		}
		float dist_sqr = to_rail.LengthSqr();
		
		if (dist_sqr > closest_dist_sqr) continue;
		
		if (!Rail_ValidInEditor(GetPos(pRailNode), GetPos(pRailNode->m_pNextLink))) continue;
		
		SHangRailData this_data;
		this_data.p_rail_node = pRailNode;
		if (!p_walk_component->FilterHangRail(GetPos(pRailNode), GetPos(pRailNode->m_pNextLink), LocalToWorldTransform(p2), LocalToWorldTransform(p1), f2, this_data, false)) continue;
		
		closest_dist_sqr = dist_sqr;
		rail_data = this_data;
		rail_found = true;
	}
	
	return rail_found;
}

bool CRailManager::RailNodesAreCoincident ( const CRailNode* p_node_a, const CRailNode* p_node_b )
{
	const float epsilon = 1.0f;
	
	if (Mth::Abs(p_node_a->m_pos[X] - p_node_b->m_pos[X]) > epsilon) return false;
	if (Mth::Abs(p_node_a->m_pos[Y] - p_node_b->m_pos[Y]) > epsilon) return false;
	if (Mth::Abs(p_node_a->m_pos[Z] - p_node_b->m_pos[Z]) > epsilon) return false;
	return true;
}

// look for another rail node within a few inches of the given node's position
bool CRailManager::CheckForCoincidentRailNode ( const CRailNode* p_node, uint32 ignore_mask, const CRailNode** pp_next_node )
{
	for (int check_node = 0; check_node<m_num_nodes; check_node++)
	{
		CRailNode *pRailNode = &mp_nodes[check_node];
		
		if (!RailNodesAreCoincident(pRailNode, p_node)) continue;
		
		if (!pRailNode->m_pNextLink && !pRailNode->m_pPrevLink) continue;
		
		if (pRailNode == p_node) continue;
		
		if (pRailNode->m_pNextLink && pRailNode->IsActive() && !pRailNode->m_flags.TestMask(ignore_mask))
		{
			// MESSAGE("found potential coincident");
			if (Rail_ValidInEditor(GetPos(pRailNode), GetPos(pRailNode->m_pNextLink)))
			{
				*pp_next_node = pRailNode;
				// MESSAGE("found good coincident");
				return true;
			}
			else
			{
				// MESSAGE("not valid coincident");
			}
		}
		
		if (pRailNode->m_pPrevLink && pRailNode->m_pPrevLink->IsActive() && !pRailNode->m_pPrevLink->m_flags.TestMask(ignore_mask))
		{
			// MESSAGE("found potential coincident");
			if (Rail_ValidInEditor(GetPos(pRailNode->m_pPrevLink), GetPos(pRailNode)))
			{
				*pp_next_node = pRailNode->m_pPrevLink;
				// MESSAGE("found good coincident");
				return true;
			}
			else
			{
				// MESSAGE("not valid coincident");
			}
		}
	}
	
	return false;
}

// see if we can stick to a rail	
bool CRailManager::StickToRail(const Mth::Vector &pos1, const Mth::Vector &pos2, Mth::Vector *p_point, CRailNode **pp_rail_node, const CRailNode *p_ignore_node, float min_dot, int side)
{
	
	// Go through all the rail segments, and find the shortest distance to line
	// and there is your rail
	
	Mth::Line  movement;
	movement.m_start = pos1;
	movement.m_end = pos2;
	
	// find bounding box for skater
	Mth::Vector bb_min, bb_max;
	Rail_ComputeBB(movement.m_start, movement.m_end, bb_min, bb_max);
	float snap_dist = Script::GetFloat(CRCD(0xf840753e, "Rail_Max_Snap"));
	bb_min.Set(bb_min.GetX() - snap_dist, bb_min.GetY() - snap_dist, bb_min.GetZ() - snap_dist);	
	bb_max.Set(bb_max.GetX() + snap_dist, bb_max.GetY() + snap_dist, bb_max.GetZ() + snap_dist);	
	
	float		closest_dist = 10000000.0f;
	CRailNode   * p_closest_rail = NULL;
	Mth::Vector		closest_point; 

	bool	found = false;
						

//	CRailNode *pRailNode = mp_first_node;
//	while (pRailNode)
	for (int check_node = 0;check_node<m_num_nodes;check_node++)
	{
		CRailNode *pRailNode = &mp_nodes[check_node];
		if (!pRailNode->GetFlag(ONLY_CLIMBING) && pRailNode != p_ignore_node && pRailNode->GetActive())
		{
			if (pRailNode->m_pNextLink)
			{
				// First do bounding box test, before time-intensive LineLineIntersect test 
				
				// *** IMPORTANT ***
				// There should be indentations for each 'if', but I left them out for readability
				if (!(bb_min.GetX() > pRailNode->m_BBMax.GetX()))
				if (!(bb_max.GetX() < pRailNode->m_BBMin.GetX()))
				if (!(bb_min.GetZ() > pRailNode->m_BBMax.GetZ()))
				if (!(bb_max.GetZ() < pRailNode->m_BBMin.GetZ()))
				if (!(bb_min.GetY() > pRailNode->m_BBMax.GetY()))
				if (!(bb_max.GetY() < pRailNode->m_BBMin.GetY()))
				{
					// we have a rail segment with a BB match
					// so see if we are close to it 
					Mth::Line segment;
					segment.m_start = pRailNode->m_pos;
					segment.m_end = pRailNode->m_pNextLink->m_pos;
	
					if (Rail_ValidInEditor(pRailNode->m_pos,pRailNode->m_pNextLink->m_pos))
					{
						Mth::Vector p1,p2;
						float  f1,f2;
						if (Mth::LineLineIntersect(movement, segment, &p1, &p2, &f1, &f2))
						{
		
							Mth::Vector	to_rail = p2-p1;
							float 	dist = to_rail.Length();
							float	old_dist = dist; 
							
							// calculate the dot product of the
							// the movement and the rail in the XZ plane
							Mth::Vector  v1,v2;
							v1 = segment.m_end - segment.m_start;
							v2 = pos2 - pos1;
							v1[Y] = 0.0f;
							v2[Y] = 0.0f;
							v1.Normalize();
							v2.Normalize();
							float dot = Mth::Abs(Mth::DotProduct(v1,v2));
	
							// if now v2 (the skater's movement vector) is all zero
							// and the dot is 0.0f
							// then that means we were going precisely straght up
							// so we set the dot to 1,
							// as normally (if we we slightly left or right)
							// we would be going along the rail at the top of the pipe
							// in the XZ plane (albeit slowly)
							if (v2[X] == 0.0f && v2[Z] == 0.0f && dot == 0.0f)
							{
								dot = 1.0f;
							}
	
	
							dist = dist * (0.122f + 1.0f-dot);		// was (50 + (4096-dist)) on PS1						
							old_dist = old_dist * (2.0f - dot);		// was (8192-dot) on PS1 
							
	//						printf ("dot=%.2f, dist=%.2f, old_dist=%.2f, min_dot=%.2f\n",dot,dist,old_dist,min_dot);
	
							if (side)
							{
								Mth::Vector vel = pos2-pos1;
								if (pRailNode->Side(vel) != side)
								{
									dist *= 2.0f;
	//								printf ("side change, dist doubled to %.2f\n",dist);
								}
							}
							
		
							if (dist < closest_dist)
							{						
								bool close = true;
								// if we have a maximum dot, then check we don't go over it
								if (min_dot != 1.0f)
								{
									if (Mth::Abs(dot) < min_dot)
									{
										close = false; 	 
									}
								}							
								if (close)
								{
									if (old_dist > snap_dist)
									{
										// there is a good rail, but too far away
										// so kill any rail we've found so far
										// and make this the new target
										closest_dist = dist;
										found = false; 
									}
									else
									{
										// good rail, and close enough
										closest_dist = dist;
										closest_point = p2;
										p_closest_rail = pRailNode;	  
										found = true; 
									}							
								}
							}						  	
						} // end if (bb_test && Mth::LineLineIntersect(movement, segment, &p1, &p2, &f1, &f2))
					} // end if (Rail_ValidInEditor(pRailNode->m_pos,pRailNode->m_pNextLink->m_pos))
				} // end whole big bounding box test
			} // end if (pRailNode->m_pNextLink)
			else if (!pRailNode->m_pPrevLink && !pRailNode->m_pNextLink)
			{
				// special logic for a single node rail
				if (!(bb_min.GetX() > pRailNode->m_BBMax.GetX()))
				if (!(bb_max.GetX() < pRailNode->m_BBMin.GetX()))
				if (!(bb_min.GetZ() > pRailNode->m_BBMax.GetZ()))
				if (!(bb_max.GetZ() < pRailNode->m_BBMin.GetZ()))
				if (!(bb_min.GetY() > pRailNode->m_BBMax.GetY()))
				if (!(bb_max.GetY() < pRailNode->m_BBMin.GetY()))
				{
					// calculate the distance from the movement to the rail point 
					Mth::Vector offset = pRailNode->m_pos - pos1;
					Mth::Vector movement_direction = (pos2 - pos1).Normalize();
					offset -= Mth::DotProduct(offset, movement_direction) * movement_direction;
					float distance = offset.Length();
					
					if (distance > snap_dist) continue;
					
					// single node rails count as twice the distance when looking for the closest rail
					if (closest_dist < 2.0f * distance) continue;
					
					closest_dist = 2.0f * distance;
					closest_point = pRailNode->m_pos;
					p_closest_rail = pRailNode;
					found = true;
				}
			}
		} // end if (active && etc)
		
		//pRailNode = pRailNode->m_pNext;
	}				 

	if (found)
	{
		// note, the line from pos1 to closest_point will not reflect the line segment found above
		// as the line segment will actually start somewhere between pos1 and pos2, and not always on pos1
//		if ( closest_dist > Script::GetFloat("Rail_Max_Snap"))
//		{
//			found = false;
//		}
//		else
		{
			*p_point = closest_point;
			*pp_rail_node = p_closest_rail;
		}
	}
	return found;	
}

// Added by Ken.
// Returns true if the passed Node is on the same rail as the rail node.
// Note, with the "merging" rails, where two nodes can link to
// a new node, then this is not guarenteed to work
//  
// Given that the rails can now form a "loop with a tail"
// we now detect loops by traversign the list with two pointers
// one moving at half the speed of the other

bool CRailNode::ProbablyOnSameRailAs(int SearchNode) const
{		
	

	// First check if this node is the required node.
	if (m_node==SearchNode)
	{
		// Well that was easy.
		return true;
	}	
	
	
	// MICK:  Modified to return true only if on the same rail segment
	
	return false;


	

	const CRailNode *p_trailing = this;
	bool		advance_trailing = false;
	
	// Scan forwards.
	CRailNode *pNode=m_pNextLink;
	while (pNode)
	{
		if (pNode->m_node==SearchNode)
		{
			// Found it.
			return true;
		}	
		if (pNode == p_trailing)
		{
			// We've got back where we started without finding
			// the node, so return false.
			return false;
		}	
		pNode=pNode->m_pNextLink;
		
		// Advance the trailing node every other time
		// we advance the search node
		if (advance_trailing)
		{
			p_trailing = p_trailing->m_pNextLink;
		}
		advance_trailing = !advance_trailing;
	}

	p_trailing = this;
	advance_trailing = false;
		
	// Didn't find anything that way, so now scan backwards.
	pNode=m_pPrevLink;
	while (pNode)
	{
		if (pNode->m_node==SearchNode)
		{
			// Found it.
			return true;
		}	
		if (pNode == p_trailing)
		{
			// We've got back where we started without finding
			// the node, so return false.
			return false;
		}	
		pNode=pNode->m_pPrevLink;
		// Advance the trailing node every other time
		// we advance the search node
		if (advance_trailing)
		{
			p_trailing = p_trailing->m_pPrevLink;
		}
		advance_trailing = !advance_trailing;

	}
	
	return false;
}


// use in-game collision checks to determine if a rail is valid
bool  Rail_ValidInEditor(Mth::Vector Start, Mth::Vector End)
{

// MAYBE TODO:  a rail should only need disqualifying if it is along the edge of a cell
// (or even more accurately, a meta-piece
// so could maybe check for that before we try to disqualify it. 
	

	if (!Ed::CParkEditor::Instance()->UsingCustomPark()) 		// is it a custom park???
	{
		#ifdef		DEBUG_EDITOR_RAILS
		printf ("not in editor\n");
		#endif
		// if not a custom park, then everything is valid.
		return true;
	}

	CFeeler		feeler;

// Find a point "Mid", which is 1/4 of the way along the rail   
	Mth::Vector	Mid = (Start + End) / 2.0f;			// Half way along
	Mid = Start + ((Mid - Start) / 2.0f);		    // quarter of the way along	
	Mid[Y] += 6.0f;									// raised up a bit

// Find a vector "Side" which is horizontal, perpendicular to the rail, and 4 inches long	
	Mth::Vector	Side = End - Start;	   			// vector along the rail
	Side[Y] = 0.0f;								// horizontal
	Side.Normalize(7.0f);						// this is half the width of the thickest rail.  Was 4.0, changed for Tokyo mega funbox
	float temp = Side[X];			 				// make perpendicular to rail
	Side[X] = Side[Z];
	Side[Z] = -temp;

												   
	
// if collision above me, left to right, then invalid
	feeler.SetLine(Mid + Side, Mid-Side);	
	if (feeler.GetCollision())
	{
		#ifdef		DEBUG_EDITOR_RAILS
		printf ("l-r above collision, invalid\n");
		feeler.DebugLine(255,0,0);
		#endif
		return false;
	}
	else
	{
		#ifdef		DEBUG_EDITOR_RAILS
		feeler.DebugLine();
		#endif
	}

// if collision above me, right to left, then invalid 	
	feeler.FlipDirection();
	if (feeler.GetCollision())
	{
		#ifdef		DEBUG_EDITOR_RAILS
		printf ("r-l above collision, invalid\n");
		feeler.DebugLine(255,0,0);
		#endif
		return false;
	}
	else
	{
		#ifdef		DEBUG_EDITOR_RAILS
		feeler.DebugLine();
		#endif
	}

#if 1

// Get a vector "Down", which is a line straight down 7 inches
	Mth::Vector	DeepBelow(0.0f,-12.0f,0.0f);	 		// six inches below the rail
	
	float	left_height 		= 0.0f;
	float 	right_height 		= 0.0f;

// find reltive height of slope (to rail) on left side
	feeler.SetLine(Mid + Side, Mid + Side + DeepBelow);				  
	if (feeler.GetCollision())
	{
		#ifdef		DEBUG_EDITOR_RAILS
		feeler.DebugLine(0,255,0);	   	// green = possible bad one side
		#endif
		left_height = feeler.GetPoint().GetY() - (Mid[Y] -6.0f);
	}
	else
	{
		return true;
	}

	feeler.SetLine(Mid - Side, Mid - Side + DeepBelow);				  
	if (feeler.GetCollision())
	{
		#ifdef		DEBUG_EDITOR_RAILS
		feeler.DebugLine(0,255,0);	   	// green = possible bad one side
		#endif
		right_height = feeler.GetPoint().GetY() - (Mid[Y]-6.0f);
	}
	else
	{
		return true;
	}
	
	
//	printf("Left = %f, Right = %f\n",left_height, right_height);
	
	if (left_height > -1.0f && right_height > -1.0f)
	{
		// both faces tilt upwards, or are roughtly horizontal
		// so return false
		return false;
	}

	// Make it so left is higher than right for subsequent tests	
	if (left_height < right_height)
	{
		float t = left_height;
		left_height = right_height;
		right_height = t;
	}
	
	// check for steep left side
	if (left_height > 1.0f)
	{
		// sloped down right side, so return
		if (right_height > -2.0f)
		{
//			printf ("Sloped, false\n");
			return false;
		}
	}


#else

// Get a vector "Down", which is a line straight down 7 inches
	Mth::Vector	DownBelow(0.0f,-7.0f,0.0f);	 		// inch below the rail
	Mth::Vector	DownAbove(0.0f,-4.0f,0.0f);			// 2 inch above the rail


	// we do fource collision checks, two on each side of the rail
	// one to a height that goes below the rail
	// and the other that goes to a height above it
	// if two or more of these return a collision
	// then the rail is invalid
	// the majority of bad rails will be eliminated with two checks

	int cols = 0;
	feeler.SetLine(Mid + Side, Mid + Side + DownBelow);				  
	if (feeler.GetCollision())
	{
		cols++;
		#ifdef		DEBUG_EDITOR_RAILS
		feeler.DebugLine(0,255,0);	   	// green = possible bad one side
		printf ("first side collision (greeen), checking other side.....\n");
		#endif
	}
	
	feeler.SetLine(Mid-Side, Mid-Side+DownBelow);				  
	if (feeler.GetCollision())
	{
		cols++;
		#ifdef		DEBUG_EDITOR_RAILS
		feeler.DebugLine(0,255,0);	   	// green = possible bad one side
		printf ("second side collision (greeen), checking other side.....\n");
		#endif
		if (cols > 1)
		{
			return false;
		}	
	}
	
	feeler.SetLine(Mid+Side, Mid+Side+DownAbove);				  
	if (feeler.GetCollision())
	{
		cols++;
		#ifdef		DEBUG_EDITOR_RAILS
		feeler.DebugLine(0,255,0);	   	// green = possible bad one side
		printf ("third side collision (greeen), checking other side.....\n");
		#endif
		if (cols > 1)
		{
			return false;
		}	
	}
	
	feeler.SetLine(Mid-Side, Mid-Side+DownAbove);				  
	if (feeler.GetCollision())
	{
		cols++;
		#ifdef		DEBUG_EDITOR_RAILS
		feeler.DebugLine(0,255,0);	   	// green = possible bad one side
		printf ("forth side collision (greeen)\n");
		#endif
		if (cols > 1)
		{
			return false;
		}	
	}
#endif			
		
// Not found two collisions
// so we can return true, indicating this rail is valid in the park editor
//	printf ("everything ok, valid\n");
	return true;
}



void	CRailManager::DebugRender(Mth::Matrix *p_transform)
{
#ifdef	__DEBUG_CODE__
	
	#ifdef	__PLAT_NGPS__	
	
	NxPs2::DMAOverflowOK = 2;
						   
	
	Mth::Vector	cam_fwd;
	
	if (Nx::CViewportManager::sGetActiveCamera())
	{
		cam_fwd = Nx::CViewportManager::sGetActiveCamera()->GetMatrix()[Z];
	}
	else
	{
		printf("WARNING: called CRailManager::DebugRender without a camera\n");
		return;
	}

	
	
	bool	draw_arrows = Script::GetInt( CRCD(0xc57f95d7,"rail_arrows"), false );

	Mth::Vector up(0.0f,1.0f,0.0f);	

	uint32	rgb = 0x0000ff;
	
	uint32 current = 12345678;

	
	
//	CRailNode *pRailNode;
//	pRailNode = mp_first_node;
	int count = 0;
	for (int check_node = 0;check_node<m_num_nodes;check_node++)
	{
		CRailNode *pRailNode = &mp_nodes[check_node];
		count++;
		CRailNode *pNext = pRailNode->m_pNextLink;
		
		int terrain = pRailNode->GetTerrain();
			
		if (pNext)
		{
		
			switch (terrain)
			{
			
				case vTERRAIN_CONCSMOOTH:
				case vTERRAIN_CONCROUGH:	
					rgb = 0x0000ff;			// red = concrete
					break;
				case vTERRAIN_METALSMOOTH:
				case vTERRAIN_METALROUGH:	
				case vTERRAIN_METALCORRUGATED:	
				case vTERRAIN_METALGRATING:
				case vTERRAIN_METALTIN: 
					rgb = 0xff0000;			// blue = metal
					break;
				 
				case vTERRAIN_WOOD:		
				case vTERRAIN_WOODMASONITE:
				case vTERRAIN_WOODPLYWOOD:
				case vTERRAIN_WOODFLIMSY:	
				case vTERRAIN_WOODSHINGLE:
				case vTERRAIN_WOODPIER:	
					rgb = 0x00ff00;			// green = wood
					break;
				default:
					rgb = 0xffffff;			// white = everything else 
					break;
			}
			
			if (pRailNode->m_flags.Test(LADDER))
			{
				rgb = 0xffff00;
			}
		

			// check for context changes
			if (current != rgb)
			{
				if ( current != 12345678)
				{
					NxPs2::EndLines3D();
				}
				
				current = rgb;
				NxPs2::BeginLines3D(0x80000000 + (0x00ffffff & rgb));
			}
		
			Mth::Vector v0, v1;
			v0 = pRailNode->m_pos/* + up*/;	  	// the +up is the lift them off the surface, so they render better
			v1 = pNext->m_pos/* + up*/;
			v0[W] = 1.0f;		// Make homgeneous
			v1[W] = 1.0f;

			if (p_transform)
			{
				v0 = p_transform->Transform(v0);
				v1 = p_transform->Transform(v1);
			}

			if (pRailNode->GetTerrain() || ((Tmr::GetTime() % 100) > 50))
			{
				NxPs2::DrawLine3D(v0[X],v0[Y],v0[Z],v1[X],v1[Y],v1[Z]);
				NxPs2::DrawLine3D(v0[X],v0[Y],v0[Z],v1[X],v1[Y],v1[Z]);
			
				if (draw_arrows)
				{
					// Calculate and draw an arrowhead 
					// 1/4th the length, at ~30 degrees					   
					Mth::Vector	a = v0;
					Mth::Vector b = v1;
					Mth::Vector	ab = (b-a);
					ab /= 4.0f;
					Mth::Vector out;
					out = ab;
					out.Normalize();
					out = Mth::CrossProduct(out, cam_fwd);
					out *= ab.Length()/3.0f;			
					
					Mth::Vector left =  b - ab + out;
					Mth::Vector right = b - ab - out;
		
					NxPs2::DrawLine3D(left[X],left[Y],left[Z],b[X],b[Y],b[Z]);
					NxPs2::DrawLine3D(right[X],right[Y],right[Z],b[X],b[Y],b[Z]);
					NxPs2::DrawLine3D(right[X],right[Y],right[Z],left[X],left[Y],left[Z]);	 // crossbar
					// have to draw it twice for some stupid reason	(final segment of a particular color is not rendered)
					NxPs2::DrawLine3D(left[X],left[Y],left[Z],b[X],b[Y],b[Z]);
					NxPs2::DrawLine3D(right[X],right[Y],right[Z],b[X],b[Y],b[Z]);
					NxPs2::DrawLine3D(right[X],right[Y],right[Z],left[X],left[Y],left[Z]);
				}		
			
			
			
			}
		}
		
//		pRailNode = pRailNode->m_pNext;
		
	}
	

	// then draw the node positions as litle red lines	
//	pRailNode = mp_first_node;
	count = 0;
	for (int check_node = 0;check_node<m_num_nodes;check_node++)
	{
		CRailNode *pRailNode = &mp_nodes[check_node];
		count++;
		if (pRailNode->GetActive())
		{
			rgb = 0xff0000;		 		// blue for active
		}
		else
		{
			rgb = 0x00ff00;				// green for inactive
		}
		
		if (!pRailNode->GetPrevLink())
		{
			rgb |= 0x0000ff;			// blue+red = cyan = no prev node (or green+red = yellow for no prev inactive)
		}
	
		// check for context changes
		if (current != rgb)
		{
			if ( current != 12345678)
			{
				NxPs2::EndLines3D();
			}
			
			current = rgb;
			NxPs2::BeginLines3D(0x80000000 + (0x00ffffff & rgb));
		}
	
		Mth::Vector v0, v1;
		v0 = pRailNode->m_pos;
		v1 = v0 + up * 24.0f;
		v0[W] = 1.0f;		// Make homgeneous
		v1[W] = 1.0f;

		if (p_transform)
		{
			v0 = p_transform->Transform(v0);
			v1 = p_transform->Transform(v1);
		}

		if (pRailNode->GetTerrain() || ((Tmr::GetTime() % 100) > 50))
		{
			NxPs2::DrawLine3D(v0[X],v0[Y],v0[Z],v1[X],v1[Y],v1[Z]);
			NxPs2::DrawLine3D(v0[X],v0[Y],v0[Z],v1[X],v1[Y],v1[Z]);
		}
//		pRailNode = pRailNode->m_pNext;
		
	}

	
	
	// only if we actually drew some
	if ( current != 12345678)
	{
		NxPs2::EndLines3D();
	}


	#endif
	#endif

}




void	CRailManager::AddRailsFromNodeArray(Script::CArray * p_nodearray)
{
	Dbg_MsgAssert(!mp_node_array,("Setting two node arrays in rail manager"));
	mp_node_array = p_nodearray;
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	
	uint32 i;	
	

	Dbg_MsgAssert(m_num_nodes == 0,("Can only addd nodes once, already %d there\n",m_num_nodes));										  
	m_num_nodes = 0;										  
										  
	// First iterate over the node array, and count the number of nodes needed


	for (i=0; i<p_nodearray->GetSize(); ++i)
	{
		Script::CStruct *p_node_struct=p_nodearray->GetStructure(i);
		Dbg_MsgAssert(p_node_struct,("Error getting node from node array for rail generation"));
		if ( !skate_mod->ShouldBeAbsentNode( p_node_struct ) )
		{
			uint32 class_checksum = 0;		
			p_node_struct->GetChecksum( 0x12b4e660 /*"Class"*/, &class_checksum );
			if (class_checksum == CRCD(0x8e6b02ad, "railnode") || class_checksum == CRCD(0x30c19600, "ClimbingNode"))
			{
				m_num_nodes++;										  
			}
			
			// if (class_checksum == CRCD(0x16d1e502, "climbnode"))
			// {
				// printf("Found climb node!\n");
			// }
		}
	}


	// No nodes found, so just return
	if (!m_num_nodes)
	{
		return;
	}
	
	mp_nodes = (CRailNode*)Mem::Malloc(m_num_nodes * sizeof(CRailNode));
	mp_links = (CRailLinks*)Mem::Malloc(m_num_nodes * sizeof(CRailLinks));

	// iterate over the nodes and add them to the array	
	m_current_node = 0;	
	
	 
	for (i=0; i<p_nodearray->GetSize(); ++i)
	{
		Script::CStruct *p_node_struct=p_nodearray->GetStructure(i);
		Dbg_MsgAssert(p_node_struct,("Error getting node from node array for rail generation"));

		if ( !skate_mod->ShouldBeAbsentNode( p_node_struct ) )
		{
	
	
			uint32 class_checksum = 0;		
			p_node_struct->GetChecksum( 0x12b4e660 /*"Class"*/, &class_checksum );
			if (class_checksum == CRCD(0x8e6b02ad, "railnode") || class_checksum == CRCD(0x30c19600, "ClimbingNode"))
			{
				
	
				AddRailNode( i, p_node_struct);
	
				bool is_trick_object = p_node_struct->ContainsComponentNamed( "TrickObject" );
	
	#if 1
				bool debug_graffiti = Script::GetInt( "create_all_trick_objects", false );
				if ( debug_graffiti )
					is_trick_object = true;
	#endif
	
				if ( is_trick_object )
				{
					// get the node name
					uint32 checksumName;
					p_node_struct->GetChecksum( "name", &checksumName, true );
	
					// must have a cluster associated with it
					uint32 clusterName = checksumName;
	#if 0
					// automatically link all rail nodes to the TestCluster for now
					clusterName = Script::GenerateCRC("TestCluster");
	#else
					p_node_struct->GetChecksum( "Cluster", &clusterName, true );
	#endif
					skate_mod->GetTrickObjectManager()->AddTrickCluster( clusterName );
	
					// bind the name of the rail node to the cluster name
					skate_mod->GetTrickObjectManager()->AddTrickAlias( checksumName, clusterName );
				}
			}
		}
	}


	
	int num_nodes  = p_nodearray->GetSize();   
	
	// we are creating a table of all nodes, and the pointer to the CRailNode
	// for that node, so we can do a reverse lookup
	
	CRailNode **pp_railnodes = (CRailNode **) Mem::Malloc(num_nodes * sizeof(CRailNode*));
	for (int i=0;i<num_nodes;i++)
	{
		pp_railnodes[i] = NULL;
	}

	// now fill it in	
	CRailNode *p_node; // = mp_first_node;
//	while (p_node)
	for (int node=0;node<m_num_nodes;node++)
	{
		p_node = &mp_nodes[node];
		pp_railnodes[p_node->GetNode()] = p_node;
//		p_node = p_node->m_pNext;
	}

	// now go through all the node	
//	p_node = mp_first_node;
	for (int node=0;node<m_num_nodes;node++)
	{
		p_node = &mp_nodes[node];
		for (int i=0;i<MAX_RAIL_LINKS;i++)
		{
			if (mp_links[node].m_link[i] != - 1)
			{
				Dbg_MsgAssert(mp_links[node].m_link[i] < num_nodes, ("Node %d,Rail link node (%d) out of range (0 .. %d). Bad Node array?",
												p_node->m_node, mp_links[node].m_link[i], num_nodes));
				Dbg_MsgAssert(pp_railnodes[mp_links[node].m_link[i]], ("RailNode %d linked to something (node %d) that is not a RailNode",
												p_node->m_node, mp_links[node].m_link[i]));
				NewLink(p_node,pp_railnodes[mp_links[node].m_link[i]]);
			}
		}
//		p_node = p_node->m_pNext;
	}
	Mem::Free(pp_railnodes);
	
	Mem::Free(mp_links);
	mp_links = NULL;
	
	//printf ("THERE ARE %d rails\n",m_num_nodes);
	
}

Mth::Vector CRailManager::LocalToWorldTransform ( const Mth::Vector& vector ) const
{
	if (!m_is_transformed)
	{
		return vector;
	}
	else
	{
		return m_transform.Transform(vector);
	}
}

Mth::Vector	CRailManager::GetPos(const CRailNode *p_node) 	const
{
	Dbg_Assert(p_node);
	
	if (!m_is_transformed)
	{
		return p_node->GetPos();
	}
	else
	{
		Mth::Vector v = m_transform.Transform(p_node->GetPos());
		return v;
	}
}

Mth::Matrix	CRailManager::GetMatrix(const CRailNode *p_node) 	const
{
	Dbg_Assert(p_node);
	
	if (!m_is_transformed)
	{
		return Mth::Matrix(p_node->GetOrientation());
	}
	else
	{
		Mth::Matrix m = m_transform * Mth::Matrix(p_node->GetOrientation());
		return m;
	}
}


Mth::Vector	CRailManager::GetBBMin(const CRailNode *p_node) const
{
	if (!m_is_transformed)
	{
		return p_node->GetBBMin();
	}
	else
	{
		Mth::Vector v = m_transform.Transform(p_node->GetBBMin());
		return v;
	}
}

Mth::Vector	CRailManager::GetBBMax(const CRailNode *p_node) const
{
	if (!m_is_transformed)
	{
		return p_node->GetBBMax();
	}
	else
	{
		Mth::Vector v = m_transform.Transform(p_node->GetBBMax());
		return v;
	}
}

// Auto-generation of rails

// a binning data structure similar to a hashtable which will allow us to quickly find nearby rail endpoints
// differs from the standard hashtable in that we will be able to add an element multiple times
// its algorithm is just as inefficient as the standard hashtable

struct SAutoRail;
class BinTable;

class BinItem
{
	friend class BinTable;

private:
	BinItem (   )
		: mp_value(NULL), mp_next(NULL)
		{   }

	SAutoRail* mp_value;
	BinItem* mp_next;
};

class BinTable
{
public:
	BinTable ( uint32 numBits );
	~BinTable (   );

	bool PutItem ( const uint32 key, SAutoRail* item );
	bool PutItemTwice ( const uint32 first_key, const uint32 second_key, SAutoRail* item );
	SAutoRail* GetFirstItem ( const uint32 key );
	SAutoRail* GetNextItem ( const uint32 key, SAutoRail* item );
	void FlushAllItems (   );

	int GetSize (   ) { return m_size; }

protected:
	uint32 m_numBits;
	BinItem* mp_table;
	int m_size;

private:
	uint32 key_to_bin ( const uint32 key )
		{ return key & ((1 << m_numBits) - 1); }
};

BinTable::BinTable ( uint32 numBits )
	: m_numBits(numBits), mp_table(new BinItem[1 << numBits]), m_size(0)
	{   }

BinTable::~BinTable (   )
{
	Dbg_AssertPtr(mp_table);

	FlushAllItems();

	delete [] mp_table;
	mp_table = NULL;
}

bool BinTable::PutItem ( const uint32 key, SAutoRail* item )
{    
    Dbg_AssertPtr(item);

	Dbg_AssertPtr(mp_table);

	Dbg_MsgAssert(key || item, ("Both key and item are 0 (NULL) in bin table"));
	
	Dbg_MsgAssert(item, ("NULL item added to bin table"));

	BinItem *pEntry = &mp_table[key_to_bin(key)];
	if (pEntry->mp_value)
	{
#ifndef __PLAT_WN32__
		BinItem *pNew = new (Mem::PoolManager::SCreateItem(Mem::PoolManager::vHASH_ITEM_POOL)) BinItem();
#else
		BinItem *pNew = new BinItem;
#endif
		pNew->mp_value = item;
		pNew->mp_next = pEntry->mp_next;
		pEntry->mp_next = pNew;
	}
	else
	{
		pEntry->mp_value = item;
	}

	m_size++;	
	return true;
}

// this feature is needed to insure a rail is never added twice to the same bin; if it were, it would become
// impossible to traverse the bin
bool BinTable::PutItemTwice ( const uint32 first_key, const uint32 second_key, SAutoRail* item )
{
	PutItem(first_key, item);
	if (key_to_bin(first_key) != key_to_bin(second_key))
		PutItem(second_key, item);
	return true;
}

SAutoRail* BinTable::GetFirstItem ( const uint32 key )
{
    Dbg_AssertPtr(mp_table);

	BinItem* pEntry = &mp_table[key_to_bin(key)];

	while (pEntry)
	{
		if (pEntry->mp_value)
		{
			return pEntry->mp_value;
		}	
		pEntry = pEntry->mp_next;
	}

	return NULL;
}

SAutoRail* BinTable::GetNextItem ( const uint32 key, SAutoRail* p_item )
{
    Dbg_AssertPtr(mp_table);

	BinItem* pEntry = &mp_table[key_to_bin(key)];

	while (pEntry)
	{
		if (pEntry->mp_value == p_item)
		{
			break;
		}	
		pEntry = pEntry->mp_next;
	}
	if (!pEntry)
	{
		return NULL;
	}	

	pEntry = pEntry->mp_next;
	while (pEntry)
	{
		if (pEntry->mp_value)
		{
			return pEntry->mp_value;
		}	
		pEntry = pEntry->mp_next;
	}

	return NULL;
}

void BinTable::FlushAllItems (   )
{
	Dbg_AssertPtr(mp_table);
	if (!mp_table)
		return;

	BinItem* pMainEntry = mp_table;
	uint32 hashTableSize = 1 << m_numBits;
	for (uint32 i = 0; i < hashTableSize; ++i)
	{
		BinItem* pLinkedEntry = pMainEntry->mp_next;
		while (pLinkedEntry)
		{
			BinItem* pNext = pLinkedEntry->mp_next;
#ifndef __PLAT_WN32__
			Mem::PoolManager::SFreeItem(Mem::PoolManager::vHASH_ITEM_POOL, pLinkedEntry);
#else
			delete pLinkedEntry;
#endif
			pLinkedEntry = pNext;
		}
		++pMainEntry;
	}	
	m_size = 0;
}

// memory managment defines; control temporary memory usage

// maximum number of edges in a sector
#define MAX_NUM_EDGES					(5000)
// maximum number of rails
#define	MAX_NUM_RAILS					(30000)
// maximum number of railsets
#define MAX_NUM_RAILSETS 				(6000)
// size of the rail endpoint bintable
#define ENDPOINT_BINTABLE_SIZE_BITS		(12) // 4096 entries

// default rail generation parameter values
// when changing these, don't forget to CHANGE THE DOCUMENTATION of the default script parameter values in cfuncs.cpp

// angle below which edges do not generate rails
#define MIN_RAIL_EDGE_ANGLE				(30.0f)
// maximum rail assent
// NOTE: can't sync with max skate slop because not a explicitly defined physics value (I think)
#define MAX_RAIL_ANGLE_OF_ASSENT		(45.0f)
// minimum length of a independent rail or a railset
#define MIN_RAILSET_LENGTH				(36.0f)
// minimum length of a rail, whether in a railset or not
#define MIN_EDGE_LENGTH					(0.0f)
// distance increment along rails at which collision detection is done
#define FEELER_INCREMENT				(36.0f)
// maximum heigh of a curb before it will get a rail 
// NOTE: sync with low-curb skate height from physics engine? i think that's 8.0f but I don't really know what I'm talking about
#define MAX_LOW_CURB_HEIGHT				(8.0f)
// angle of assent above face of low-curb feelers
#define LOW_CURB_FEELER_ANGLE_OF_ASSENT	(30.0f)
// maximum angle between two connected rails in a railset
#define MAX_CORNER_IN_RAILSET			(50.0f)
// height of vertical feeler
#define VERTICAL_FEELER_LENGTH			(5.0f * 12.0f)
// distance above rail at which crossbar feeler is used
#define CROSSBAR_FEELER_ELEVATION		(12.0f)
// half length of crossbar feeler
#define CROSSBAR_FEELER_LENGTH			(1.0f * 12.0f)
// farthest distance between an auto-generated rail and an old rail for which the auto-generated rail will be culled
#define FARTHEST_DEGENERATE_RAIL		(12.0f)
// angle between an old and and auto-generated rail below which they may be considered degenerate
#define MAX_DEGENERATE_RAIL_ANGLE		(20.0f)
// distance below which verticies are concidered equal
#define CONNECTION_SLOP					(0.1f)


// feeler usage bits
#define VERTICAL_FEELER_BIT				(1 << 0)
#define CROSSBAR_FEELER_BIT				(1 << 1)
#define LOW_CURB_FEELER_BIT				(1 << 2)
#define ALL_FEELERS_ON					(-1)

struct SEdge {
	// has a match for this edge been found?
	bool matched;

	// edge is from a vert poly; maybe I should just save all the face flag bits
	bool vert;

	// edge endpoints
	Mth::Vector p0, p1;

	// edge's face's unit normal
	Mth::Vector	normal;

	// edge's unit perp vector along face
	Mth::Vector perp;
};

struct SAutoRailEndpoint {
	// endpoint position
	Mth::Vector p;

	// connection at this end; -1 is no connection
	int connection;
};

struct SAutoRail {
	// rail endpoints
	SAutoRailEndpoint endpoints[2];

	// unit vector along rail
	Mth::Vector	para;

	// railset id; -1 for solo rail
	int railset;

	// rail length
	float length;

	// if found to be too sort or once added to the rail nodes, a rail is disabled
	bool disabled;
};

// dereference endpoints
enum { START = 0, END };

struct SRailSet {
	// length of rail set
	float length;
};

// collect the autorail state into a single structure to ease transportation
struct SAutoRailGeneratorState {
	SEdge* p_edges;
	SAutoRail* p_rails;
	SRailSet* p_railsets;

	// hash of rail endpoints; most rails are in hash twice, keyed off endpoint's X*Y rounded to nearest inch
	BinTable* p_rail_endpoint_list;

	int	num_rails;	
	int num_active_rails;
	int num_railsets;
};

// collect all the algorithm's input parameters into a structure
// see default values for explaination of meanings
struct SAutoRailGeneratorParameters {
	// input parameters
	float min_rail_edge_angle;
	float max_rail_angle_of_assent;
	float min_railset_length;
	float min_edge_length;
	float feeler_increment;
	float max_low_curb_height;
	float curb_feeler_angle_of_assent;
	float max_corner_in_railset;
	float vertical_feeler_length;
	float crossbar_feeler_elevation;
	float half_crossbar_feeler_length;
	float farthest_degenerate_rail;
	float max_degenerate_rail_angle;
	float connection_slop;
	int feeler_usage;

	// dependent parameters
	float sin_max_rail_angle_of_assent;
	float low_curb_feeler_length;
	float cos_min_rail_edge_angle;
	float cos_max_corner_in_railset;
	float sin_curb_feeler_angle_of_assent;
	float cos_curb_feeler_angle_of_assent;
	float cos_max_degenerate_rail_angle;

	// additional flags
	bool remove_old_rails;
};

inline bool very_close ( Mth::Vector a, Mth::Vector b, const SAutoRailGeneratorParameters& arp )
{
	return(Mth::Abs(a[X]-b[X]) < arp.connection_slop	
		&& Mth::Abs(a[Y]-b[Y]) < arp.connection_slop
		&& Mth::Abs(a[Z]-b[Z]) < arp.connection_slop
	);
}

inline int generate_endpoint_key ( Mth::Vector& end_point )
{
	return static_cast<int>((end_point[X] * end_point[Z]) * (1.0f / 6.0f));
}

// checks for duplicate rails and then adds rails to rail array and endpoint bintable
inline bool add_rail ( const Mth::Vector& pa, const Mth::Vector& pb, SAutoRailGeneratorState& arg, const SAutoRailGeneratorParameters& arp )
{
	// cull out duplicate rails
	// NOTE: what is causing these? some levels do not have them; perhaps sloppy geometry
	for (int n = arg.num_rails; n--; )
	{
		// if this is a duplicate, bail
		if (very_close(arg.p_rails[n].endpoints[START].p, pa, arp) && very_close(arg.p_rails[n].endpoints[END].p, pb, arp)
			|| very_close(arg.p_rails[n].endpoints[START].p, pb, arp) && very_close(arg.p_rails[n].endpoints[END].p, pa, arp))
		{
			return true;
		}
	}

	// if we get to here, we've decided this is a valid rail
	
	if (arg.num_rails + 1 == MAX_NUM_RAILS) return false;

	// add rail to rail array

	arg.p_rails[arg.num_rails].endpoints[START].p = pa;
	arg.p_rails[arg.num_rails].endpoints[END].p = pb;
	arg.p_rails[arg.num_rails].endpoints[START].connection = -1;
	arg.p_rails[arg.num_rails].endpoints[END].connection = -1;
	arg.p_rails[arg.num_rails].railset = -1;
	arg.p_rails[arg.num_rails].disabled = false;

	arg.p_rails[arg.num_rails].para = pb - pa;
	arg.p_rails[arg.num_rails].length = arg.p_rails[arg.num_rails].para.Length();
	arg.p_rails[arg.num_rails].para.Normalize();

	// add rail to endpoint bintable
	arg.p_rail_endpoint_list->PutItemTwice(
		generate_endpoint_key(arg.p_rails[arg.num_rails].endpoints[START].p),
		generate_endpoint_key(arg.p_rails[arg.num_rails].endpoints[END].p),
		&arg.p_rails[arg.num_rails]
	);

	arg.num_rails++;
	
	return true;
}

// handles nearby collision detection
// use feelers to insure that this is a good rail; if the full rail doesn't work, attempt to snip it up into smaller
// valid rails
inline bool consider_rail ( const Mth::Vector& pa, const Mth::Vector& pb, int matched_edge, const Mth::Vector& para, const Mth::Vector& normal,
	const Mth::Vector& perp, float edge_length, SAutoRailGeneratorState& arg, SAutoRailGeneratorParameters& arp )
{
	// four feelers are used at each step along the edge
	// 1, 2) short feelers up from the edge's two faces
	// 3) longer feeler straight up from the edge
	// 4) medium feeler crossbar on feeler 3

	// there are full_step_length + 2 feeler points along an edge
	// 0 corresponds the the edge's start point
	// n corresponds to a points n steps in from the start point
	// full_step_length + 1 corresponds to the edge's end point
	int full_step_length = static_cast<int>(edge_length / arp.feeler_increment);

	int start_step = 0;
	Mth::Vector r0 = pa;
	// attempt to create rail snippets until we've used up the rail
	do
	{
		// start a new rail snippet
		int end_step = start_step;
		Mth::Vector r1 = r0;

		// attempt to extent the rail snippet along the edge
		do
		{
			bool fail = false;

			// check for nearby collidables

			CFeeler feeler;
            
			// first side feeler
			// angled up from face in perp-normal plane starting a fraction of an inch from edge
			if ((arp.feeler_usage & LOW_CURB_FEELER_BIT))
			{
				Mth::Vector feeler_direction = arp.sin_curb_feeler_angle_of_assent * normal + arp.cos_curb_feeler_angle_of_assent * perp;

				feeler.m_start = r1 + 0.1f * feeler_direction;
				feeler.m_end = r1 + arp.low_curb_feeler_length * feeler_direction;

				fail = feeler.GetCollision(false);
			}

			// second side feeler
			// angled up from face in perp-normal plane starting a fraction of an inch from edge
			if (!fail && (arp.feeler_usage & LOW_CURB_FEELER_BIT))
			{
				Mth::Vector feeler_direction = arp.sin_curb_feeler_angle_of_assent * arg.p_edges[matched_edge].normal
					+ arp.cos_curb_feeler_angle_of_assent * arg.p_edges[matched_edge].perp;

				feeler.m_start = r1 + 0.2f * feeler_direction;
				feeler.m_end = r1 + arp.low_curb_feeler_length * feeler_direction;

				fail = feeler.GetCollision(false);
			}

			// vertical feeler
			// straight up starting a fraction of an inch above the edge
			if (!fail && (arp.feeler_usage & VERTICAL_FEELER_BIT))
			{
				feeler.m_start = r1;
				feeler.m_start[Y] += 0.2f;

				feeler.m_end = r1;
				feeler.m_end[Y] += arp.vertical_feeler_length;
			}

			// crossbar feeler
			// some height above rail, flush with XZ plane, perp to edge
			if (!fail && (arp.feeler_usage & CROSSBAR_FEELER_BIT))
			{
				Mth::Vector bar;

				bar[X] = para[Z];
				bar[Y] = 0.0f;
				bar[Z] = -para[X];
				bar.Normalize();

				feeler.m_start = r1;
				feeler.m_start[Y] += arp.crossbar_feeler_elevation;
				feeler.m_start += arp.half_crossbar_feeler_length * bar;

				feeler.m_end = r1;
				feeler.m_end[Y] += arp.crossbar_feeler_elevation;
				feeler.m_end += -arp.half_crossbar_feeler_length * bar;

				fail = feeler.GetCollision(false);
			}

			// if there is collidables, the extention attempt failed
			if (fail) break;

			// there are no nearby collidables
			// so we'll attempt to stretch the snippet one step or to the end of the edge
			end_step++;

			// if we're beyond the end of the edge, we're done
			if (end_step == full_step_length + 2) break;

			if (end_step == full_step_length + 1)
			{
				r1 = pb;
			}
			else
			{
				r1 = pa + end_step * arp.feeler_increment * para;
			}
		} while (true);

		// restore r1 to the last value it had before the failed extention
		end_step--;
		if (end_step == full_step_length + 1)
			r1 = pb;
		else
			r1 = pa + end_step * arp.feeler_increment * para;

		// the snippet is now as long as it can be

		// if the snippet is long enough
		if (end_step - start_step > 0 && (r0 - r1).LengthSqr() >= (arp.min_edge_length * arp.min_edge_length))
		{
			// use it
			if (!add_rail(r0, r1, arg, arp)) return false;
		}

		// jump to next start point
		start_step = end_step + 2;
		r0 = pa + start_step * arp.feeler_increment * para;

	} while (start_step < full_step_length + 1);

	return true;
}

CRailNode* CRailManager::GetRailNodeByNodeNumber( int node_num )
{
	int i;

	if( mp_nodes )
	{
		for( i = 0; i < m_num_nodes; i++ )
		{
			if( mp_nodes[i].GetNode() == node_num )
			{
				return &mp_nodes[i];
			}
		}
	}

	return NULL;
}

// knowns issues or ideas:
// snap rail endpoints - do we want to snap the endpoints of railset together; if so, definitely make the slop parameter adjustable
// vertical feeler - the vertical feeler is causing random snipping up of rails in odd places
// don't destory old rails - instead, when considering a rail, check for nearly parallel rails and don't add a new rail if you find one; bin old rails first
// cross-section edges finding - look for potential rails by joining up across sectors; time consuming; edge endpoints won't match, so we have to look for overlapping edges instead of just looking for matching endpoints
void CRailManager::AutoGenerateRails ( Script::CStruct* pParams )
{
// 27K
#ifdef	__DEBUG_CODE__
	printf("-----------------------\n");

	// let's use the Debug heap, since we might need a lot of memory
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());

	// auto-generator's state
	SAutoRailGeneratorState arg;

	arg.num_rails = 0;	
	arg.num_railsets = 0;
	arg.num_active_rails = 0;

	arg.p_edges = new SEdge[MAX_NUM_EDGES];
	arg.p_rails = new SAutoRail[MAX_NUM_RAILS];
	arg.p_railsets = new SRailSet[MAX_NUM_RAILSETS];

	arg.p_rail_endpoint_list = new BinTable(ENDPOINT_BINTABLE_SIZE_BITS);

	// auto-generator's modifiable parameters
	SAutoRailGeneratorParameters arp;

	// set parameters to defaults
	arp.min_rail_edge_angle = MIN_RAIL_EDGE_ANGLE;
	arp.max_rail_angle_of_assent = MAX_RAIL_ANGLE_OF_ASSENT;
	arp.min_railset_length = MIN_RAILSET_LENGTH;
	arp.min_edge_length = MIN_EDGE_LENGTH;
	arp.feeler_increment = FEELER_INCREMENT;
	arp.max_low_curb_height = MAX_LOW_CURB_HEIGHT;
	arp.curb_feeler_angle_of_assent = LOW_CURB_FEELER_ANGLE_OF_ASSENT;
	arp.max_corner_in_railset = MAX_CORNER_IN_RAILSET;
	arp.vertical_feeler_length = VERTICAL_FEELER_LENGTH;
	arp.crossbar_feeler_elevation = CROSSBAR_FEELER_ELEVATION;
	arp.half_crossbar_feeler_length = CROSSBAR_FEELER_LENGTH;
	arp.farthest_degenerate_rail = FARTHEST_DEGENERATE_RAIL;
	arp.max_degenerate_rail_angle = MAX_DEGENERATE_RAIL_ANGLE;
	arp.connection_slop = CONNECTION_SLOP;

	// look for overrides in script parameters
	pParams->GetFloat("min_rail_edge_angle", &arp.min_rail_edge_angle, Script::NO_ASSERT);
	pParams->GetFloat("max_rail_angle_of_assent", &arp.max_rail_angle_of_assent, Script::NO_ASSERT);
	pParams->GetFloat("min_railset_length", &arp.min_railset_length, Script::NO_ASSERT);
	pParams->GetFloat("min_edge_length", &arp.min_edge_length, Script::NO_ASSERT);
	pParams->GetFloat("feeler_increment", &arp.feeler_increment, Script::NO_ASSERT);
	pParams->GetFloat("max_low_curb_height", &arp.max_low_curb_height, Script::NO_ASSERT);
	pParams->GetFloat("curb_feeler_angle_of_assent", &arp.curb_feeler_angle_of_assent, Script::NO_ASSERT);
	pParams->GetFloat("max_corner_in_railset", &arp.max_corner_in_railset, Script::NO_ASSERT);
	pParams->GetFloat("vertical_feeler_length", &arp.vertical_feeler_length, Script::NO_ASSERT);
	pParams->GetFloat("crossbar_feeler_elevation", &arp.crossbar_feeler_elevation, Script::NO_ASSERT);
	pParams->GetFloat("crossbar_feeler_length", &arp.half_crossbar_feeler_length, Script::NO_ASSERT);
	pParams->GetFloat("farthest_degenerate_rail", &arp.farthest_degenerate_rail, Script::NO_ASSERT);
	pParams->GetFloat("max_degenerate_rail_angle", &arp.max_degenerate_rail_angle, Script::NO_ASSERT);
	pParams->GetFloat("connection_slop", &arp.connection_slop, Script::NO_ASSERT);

	arp.half_crossbar_feeler_length /= 2.0f;
	arp.low_curb_feeler_length = arp.max_low_curb_height / cosf(DEGREES_TO_RADIANS(arp.curb_feeler_angle_of_assent));
	arp.sin_max_rail_angle_of_assent = sinf(DEGREES_TO_RADIANS(arp.max_rail_angle_of_assent));
	arp.cos_min_rail_edge_angle = cosf(DEGREES_TO_RADIANS(arp.min_rail_edge_angle));
	arp.cos_max_corner_in_railset = cosf(DEGREES_TO_RADIANS(arp.max_corner_in_railset));
	arp.sin_curb_feeler_angle_of_assent = sinf(DEGREES_TO_RADIANS(arp.curb_feeler_angle_of_assent));
	arp.cos_curb_feeler_angle_of_assent = cosf(DEGREES_TO_RADIANS(arp.curb_feeler_angle_of_assent));
	arp.cos_max_degenerate_rail_angle = cosf(DEGREES_TO_RADIANS(arp.max_degenerate_rail_angle));

	// turn on all feelers by default
	arp.feeler_usage = ALL_FEELERS_ON;

	// check for overrides in script parameters
	if (pParams->ContainsFlag("no_vertical_feeler"))
		arp.feeler_usage &= ~VERTICAL_FEELER_BIT;
	if (pParams->ContainsFlag("no_crossbar_feeler"))
		arp.feeler_usage &= ~CROSSBAR_FEELER_BIT;
	if (pParams->ContainsFlag("no_low_curb_feeler"))
		arp.feeler_usage &= ~LOW_CURB_FEELER_BIT;
	if (pParams->ContainsFlag("no_feelers"))
		arp.feeler_usage = 0;

	// check additional flags
	arp.remove_old_rails = false;
	if (pParams->ContainsFlag("overwrite"))
	{
		arp.remove_old_rails = true;
	}

	// dump parameter state

	printf("min_rail_edge_angle = %f\n", arp.min_rail_edge_angle);
	printf("max_rail_angle_of_assent = %f\n", arp.max_rail_angle_of_assent);
	printf("min_railset_length = %f\n", arp.min_railset_length);
	printf("min_edge_length = %f\n", arp.min_edge_length);
	printf("connection_slop = %f\n", arp.connection_slop);
	printf("feeler_increment = %f\n", arp.feeler_increment);
	printf("max_low_curb_height = %f\n", arp.max_low_curb_height);
	printf("curb_feeler_angle_of_assent = %f\n", arp.curb_feeler_angle_of_assent);
	printf("max_corner_in_railset = %f\n", arp.max_corner_in_railset);
	printf("vertical_feeler_length = %f\n", arp.vertical_feeler_length);
	printf("crossbar_feeler_length = %f\n", 2.0f * arp.half_crossbar_feeler_length);
	printf("crossbar_feeler_elevation = %f\n", arp.crossbar_feeler_elevation);

	if (!arp.remove_old_rails)
	{
		printf("farthest_degenerate_rail = %f\n", arp.farthest_degenerate_rail);
		printf("max_degenerate_rail_angle = %f\n", arp.max_degenerate_rail_angle);
	}

	if (!(arp.feeler_usage & VERTICAL_FEELER_BIT))
		printf("vertical feeler deactivated\n");
	if (!(arp.feeler_usage & CROSSBAR_FEELER_BIT))
		printf("crossbar feeler deactivated\n");
	if (!(arp.feeler_usage & LOW_CURB_FEELER_BIT))
		printf("low-curb feeler deactivated\n");
	if (arp.feeler_usage == ALL_FEELERS_ON)
		printf("all feelers active\n");

	if (arp.remove_old_rails)
	{
		printf("removing existing rails\n");
	}
	else
	{
		printf("retaining existing rails\n");
	}

	printf("generating rails...\n");

	Nx::CScene *p_scene = Nx::CEngine::sGetMainScene();
	Lst::HashTable<Nx::CSector>* p_sector_list = p_scene->GetSectorList();
	if (!p_sector_list)
		return;

	// loop through every sector in the scene
	// NOTE: to do a better job, we may need to attempt to match edges between sectors
	// if so, we'd have to deal with the issue of not being able to expect edge vertices to match up
	p_sector_list->IterateStart();	
	Nx::CSector *p_sector = p_sector_list->IterateNext();		
	while(p_sector)
	{
		Nx::CCollObjTriData *p_coll_obj = p_sector->GetCollSector()->GetGeometry();
		if (!p_coll_obj || !p_sector->IsActive() || !p_sector->IsCollidable())
		{
			p_sector = p_sector_list->IterateNext();		
			continue;
		}

		// we have the collision data, now build a list of edges
		// based solely on positions of verts

		int	num_edges = 0;

		Mth::Vector p[3];
		
		// loop though the faces of the sector
		int num_faces = p_coll_obj->GetNumFaces();
		for (int face = 0; face < num_faces; face++)
		{
			// get the normal													 
			Mth::Vector normal = p_coll_obj->GetFaceNormal(face);
			
			// Get the three vertex indicies for this face
			// and the vertex position into the above arrays
			int v[3];
            for (int i = 0; i < 3; i++)			
			{
				v[i] = p_coll_obj->GetFaceVertIndex(face,i);
				p[i] = p_coll_obj->GetRawVertexPos(v[i]);
					
			}
			// iterate over the edges (0->1, 1->2, 2->0)
			for (int a = 0; a < 3; a++)	   		// 'a' is the first point on the edge
			{				
				int b = (a + 1) % 3;			// 'b' is the second point on the edge
				int c = (b + 1) % 3;			// 'c' is the opposite point on the triangle
				// we are considering the edge a->b

				// generate a unit vector along the edge from a to b, from start to end
				Mth::Vector	para = (p[b] - p[a]);
				float edge_length = para.Length();
				if (edge_length != 0.0f) para /= edge_length; // normalize by hand since we have the length

				// cull certain edges at this point; we don't cull faces at this point in
				// order to better match edges

				// cull very short edges
				if (edge_length < arp.min_edge_length) continue;

				// use edges only if less than some angle from horizontal
				if (Mth::Abs(para[Y]) > arp.sin_max_rail_angle_of_assent) continue;

				// generate a unit vector along the triangle face perpendicular to the edge
				Mth::Vector perp = (p[c] - p[a]);
				perp -= Mth::DotProduct(perp, para) * para;
				perp.Normalize();

				// check to see if we already have this edge's match; bail if we find a match
				bool new_edge = true;
				for (int edge = 0; edge < num_edges && new_edge; edge++)
				{
					if (arg.p_edges[edge].matched) continue;
				
					// check for matched edge 
					if (very_close(arg.p_edges[edge].p0, p[a], arp) && very_close(arg.p_edges[edge].p1, p[b], arp)
						|| 	very_close(arg.p_edges[edge].p0, p[b], arp) && very_close(arg.p_edges[edge].p1, p[a], arp))
					{
						new_edge = false;
						arg.p_edges[edge].matched = true;
						
						// we've found a match, let's see if the edge is a good rail
						{
							// ignore upside down faces			
							// NOTE: we could probably come up with a better heuristic using both edges' normals
							if (normal[Y] < -0.707 || arg.p_edges[edge].normal[Y] < -0.707) break;

							// ignore face if non-collidable
							if (p_coll_obj->GetFaceFlags(face) & mFD_NON_COLLIDABLE) break;
							
							// exclude roughly coplanar polygons
							if (Mth::DotProduct(normal, arg.p_edges[edge].normal) > arp.cos_min_rail_edge_angle) break;
														
							// cull out interior edges; that is, if sum of the normals points along the sum of the perps
							if (Mth::DotProduct(perp + arg.p_edges[edge].perp, normal + arg.p_edges[edge].normal) > 0.0f) break;

							// we don't want the edges of verts to generate rails, only the tops; so cull non-horizontal vert rails
							if ((arg.p_edges[edge].vert || (p_coll_obj->GetFaceFlags(face) & mFD_VERT)) && Mth::Abs(para[Y]) > 0.1f) break;

							// if we haven't removed the old rails
							if (!arp.remove_old_rails)
							{
								// loop over all old rails and check for degeneracy
								int check_node = 0;
								for (check_node = 0; check_node < m_num_nodes; check_node++)
								{
									CRailNode *pRailNode = &mp_nodes[check_node];
	
									if (!pRailNode->m_pNextLink) continue;
	
									// is the new rail within a loose bounding box of the old rail
									if (p[a][X] < pRailNode->m_BBMin[X] - arp.farthest_degenerate_rail) continue;
									if (p[a][Y] < pRailNode->m_BBMin[Y] - arp.farthest_degenerate_rail) continue;
									if (p[a][Z] < pRailNode->m_BBMin[Z] - arp.farthest_degenerate_rail) continue;
									if (p[a][X] > pRailNode->m_BBMax[X] + arp.farthest_degenerate_rail) continue;
									if (p[a][Y] > pRailNode->m_BBMax[Y] + arp.farthest_degenerate_rail) continue;
									if (p[a][Z] > pRailNode->m_BBMax[Z] + arp.farthest_degenerate_rail) continue;
									if (p[b][X] < pRailNode->m_BBMin[X] - arp.farthest_degenerate_rail) continue;
									if (p[b][Y] < pRailNode->m_BBMin[Y] - arp.farthest_degenerate_rail) continue;
									if (p[b][Z] < pRailNode->m_BBMin[Z] - arp.farthest_degenerate_rail) continue;
									if (p[b][X] > pRailNode->m_BBMax[X] + arp.farthest_degenerate_rail) continue;
									if (p[b][Y] > pRailNode->m_BBMax[Y] + arp.farthest_degenerate_rail) continue;
									if (p[b][Z] > pRailNode->m_BBMax[Z] + arp.farthest_degenerate_rail) continue;
	
									// is the new rail parallel to the old rail
									Mth::Vector old_para = pRailNode->m_pNextLink->m_pos - pRailNode->m_pos;
									old_para.Normalize();
									if (Mth::Abs(Mth::DotProduct(para, old_para)) < arp.cos_max_degenerate_rail_angle) continue;
	
									// if the perpendicular distance from the start of the new rail to the start of the old rail is small, it's degenerate
									perp = p[a] - pRailNode->m_pos;
									perp -= Mth::DotProduct(para, perp) * para;
									if (perp.LengthSqr() < (arp.farthest_degenerate_rail * arp.farthest_degenerate_rail)) break;
	
									// if the perpendicular distance from the end of the new rail to the start of the old rail is small, it's degenerate
									perp = p[b] - pRailNode->m_pos;
									perp -= Mth::DotProduct(para, perp) * para;
									if (perp.LengthSqr() < (arp.farthest_degenerate_rail * arp.farthest_degenerate_rail)) break;
	
									// if the perpendicular distance from the start of the new rail to the end of the old rail is small, it's degenerate
									perp = p[a] - pRailNode->m_pNextLink->m_pos;
									perp -= Mth::DotProduct(para, perp) * para;
									if (perp.LengthSqr() < (arp.farthest_degenerate_rail * arp.farthest_degenerate_rail)) break;
	
									// if the perpendicular distance from the end of the new rail to the end of the old rail is small, it's degenerate
									perp = p[b] - pRailNode->m_pNextLink->m_pos;
									perp -= Mth::DotProduct(para, perp) * para;
									if (perp.LengthSqr() < (arp.farthest_degenerate_rail * arp.farthest_degenerate_rail)) break;
								}

								// if we broke from the loop, cull the rail due to degeneracy
								if (check_node != m_num_nodes) break;
							}

							// use feelers to test for good rails within this edge
							if (!consider_rail(p[a], p[b], edge, para, normal, perp, edge_length, arg, arp))
							{
								printf("failed: more than %d rails\n", MAX_NUM_RAILS);
								goto ABORT;
							}

							
						} // END valid rail test block
					} // END if (this edge matches)
				} // END loop over previous edges looking for match
				
				// if we didn't find a match
				if (new_edge)
				{
					if (num_edges + 1 < MAX_NUM_EDGES)
					{
						// save the new edge
						arg.p_edges[num_edges].p0 = p[a];
						arg.p_edges[num_edges].p1 = p[b];
						arg.p_edges[num_edges].normal = normal;
						arg.p_edges[num_edges].perp = perp;
						arg.p_edges[num_edges].matched = false;
						arg.p_edges[num_edges].vert = p_coll_obj->GetFaceFlags(face) & mFD_VERT;

						num_edges++;
					}
					else
					{
						printf("failed: more that %d edges in an object\n", MAX_NUM_EDGES);
						goto ABORT;	// sorry dijkstra ...
					}
				} // END if (new_edge)
			} // END for (int a=0;a<3;a++) (iterating over 3 edges in a face)
		} // END for (int face = 0; face < num_faces; face++) (iterate over faces in a collision sector)

		p_sector = p_sector_list->IterateNext();
	} // END loop over sectors

	printf("building railsets...\n");

	// we need to group in the individual rails into continuous rail sets, so we can cull short solo rails and short railsets
	// note that the connectivity is not perfect in the sense that it can make loops but not loops with tails

	// loop through rail set
	for (int r = 0; r < arg.num_rails; r++)
	{
		SAutoRail& rail = arg.p_rails[r];

		// loop over rail's endpoints
		for (int endpoint = START; endpoint <= END; endpoint++)
		{
			// if the endpoint is connected
			if (rail.endpoints[endpoint].connection != -1) continue;

			// now we'll check for connections

			int rail_key = generate_endpoint_key(rail.endpoints[endpoint].p);
	
			// loop over all rails in same endpoint bin as rail's endpoint's bin
			// we will check only these rails for connections
			for (SAutoRail* next_rail = arg.p_rail_endpoint_list->GetFirstItem(rail_key);
				next_rail;
				next_rail = arg.p_rail_endpoint_list->GetNextItem(rail_key, next_rail))
			{
				SAutoRail& match_rail = *next_rail;

				// we'll always find ourself
				if (&match_rail == &rail) continue;
	
				// check only rails with unconnected endpoints
				if (match_rail.endpoints[START].connection != -1 && match_rail.endpoints[END].connection != -1) continue;

				// rails connect only if the angle between them is small; this should be adjusted or adjustable
				float dot = Mth::DotProduct(rail.para, match_rail.para);
				if (Mth::Abs(dot) < arp.cos_max_corner_in_railset) continue;

				bool reverse_connection = false;

				// see if the start of the match rail is connected to our endpoint
				if (match_rail.endpoints[START].connection == -1
					&& very_close(rail.endpoints[endpoint].p, match_rail.endpoints[START].p, arp))
				{
					// rails connect only for obtuse angles
					if (endpoint == START)
					{
						reverse_connection = true;
						if (dot > 0.0f) continue;
					}
					else
					{
						if (dot < 0.0f) continue;
					}

					rail.endpoints[endpoint].connection = &match_rail - arg.p_rails;
					match_rail.endpoints[START].connection = r;
				}

				// else see if the end of the match rail is connected to our endpoint
				else if (match_rail.endpoints[END].connection == -1
					&& very_close(rail.endpoints[endpoint].p, match_rail.endpoints[END].p, arp))
				{
					// rails connect only for obtuse angles
					if (endpoint == END)
					{
						reverse_connection = true;
						if (dot > 0.0f) continue;
					}
					else
					{
						if (dot < 0.0f) continue;
					}
                    
					rail.endpoints[endpoint].connection = &match_rail - arg.p_rails;
					match_rail.endpoints[END].connection = r;
				} // END ifelse determining connectivity

				// if this endpoint of the rail is not found to be connected, continue
				if (rail.endpoints[endpoint].connection == -1) continue;

				// otherwise, we'll add the rail to a railset

				// if both rails are not in railset
				if (match_rail.railset == -1 && rail.railset == -1)
				{
					if (arg.num_railsets == MAX_NUM_RAILSETS)
					{
						printf("failed: more that %d railsets\n", MAX_NUM_RAILSETS);
						goto ABORT;
					}

					// setup a new railset
					rail.railset = arg.num_railsets;
					arg.p_railsets[arg.num_railsets].length = rail.length;
					arg.num_railsets++;

					// add match_rail to rail's new railset
					match_rail.railset = rail.railset;
					arg.p_railsets[match_rail.railset].length += match_rail.length;

					// swap the rail's endpoints if the connection is reverse
					if (reverse_connection)
					{
						SAutoRailEndpoint temp = rail.endpoints[START];
						rail.endpoints[START] = rail.endpoints[END];
						rail.endpoints[END] = temp;
					}

				// if only match_rail has a railset
				}
				else if (match_rail.railset != -1 && rail.railset == -1)
				{
					// add rail to match_rail's railset
					rail.railset = match_rail.railset;
					arg.p_railsets[rail.railset].length += rail.length;

					// swap the rail's endpoints if the connection is reverse
					if (reverse_connection)
					{
						SAutoRailEndpoint temp = rail.endpoints[START];
						rail.endpoints[START] = rail.endpoints[END];
						rail.endpoints[END] = temp;
					}

				// if only rail has a railset
				}
				else if (match_rail.railset == -1 && rail.railset != -1)
				{
					// add match_rail to rail's railset
					match_rail.railset = rail.railset;
					arg.p_railsets[match_rail.railset].length += rail.length;

					// swap the match rail's endpoints if the connection is reverse
					if (reverse_connection)
					{
						SAutoRailEndpoint temp = match_rail.endpoints[START];
						match_rail.endpoints[START] = match_rail.endpoints[END];
						match_rail.endpoints[END] = temp;
					}

				// if both rails have multrails
				}
				else
				{
					// join railsets
					// NOTE: this currently waists a railset; more flexible railset data structure would fix this;
					// can i use STL? if geometry is ordered in any reasonable fashion, waist should be minimal

					// match_rail's railset survives
					if (match_rail.railset != rail.railset)
					{
						match_rail.length += arg.p_railsets[rail.railset].length;
						for (int n = arg.num_rails; n--; )
						{
							if (arg.p_rails[n].railset == rail.railset) {
								arg.p_rails[n].railset = match_rail.railset;
							}
						}
						rail.railset = match_rail.railset;
					}

					// swap all of the rail's railset's endpoints if the connection is reverse
					// if we're closing a ring, the connection should never be reversed; so that shouldn't be a worry
					if (reverse_connection)
					{
						// traverse in the opposite direction of the connection
						int traversal_direction = endpoint ^ 1;

						// traverse the rail's railset
						for (int s = r; s != -1; s = arg.p_rails[s].endpoints[traversal_direction].connection)
						{
							SAutoRailEndpoint temp = arg.p_rails[s].endpoints[START];
							arg.p_rails[s].endpoints[START] = arg.p_rails[s].endpoints[END];
							arg.p_rails[s].endpoints[END] = temp;
						}
					}
				} // END ifelse block determining rail and match_rail's previous connectivity
			} // END loop over potential match rails

		} // END loop over endpoints
	} // END loop over all rails

	printf("culling short rails...\n");

	// now we have rails grouped into railsets; we can cull rails based on their length or the length of their railset
	for (int r = 0; r < arg.num_rails; r++)
	{
		SAutoRail& rail = arg.p_rails[r];

		// solo rails
		if (rail.railset == -1)
		{
			// cull short solo rails
			if (rail.length < arp.min_railset_length)
			{
				rail.disabled = true;
				continue;
			}
		}
		// railsets
		else
		{
			// cull short railsets
			if (arg.p_railsets[rail.railset].length < arp.min_railset_length)
			{
				rail.disabled = true;
				continue;
			}
		}

		arg.num_active_rails++;
	} // END loop over rails

	if (!arp.remove_old_rails && arg.num_rails == 0)
	{
		printf("failed: no rails found\n");
		goto ABORT;
	}

	printf("creating rail nodes...\n");

	// block to scope the final-rail-list generation variables
	{
		// we need to count the number of nodes required for the new rails
		// new_num_nodes = (num rails) + (num rails with no end connection)	+ (num old nodes)
		int new_num_nodes = arg.num_active_rails;
		for (int r = arg.num_rails; r--; )
		{
			if (!arg.p_rails[r].disabled && arg.p_rails[r].endpoints[END].connection == -1)
			{
				new_num_nodes++;
			}
		}
		if (!arp.remove_old_rails)
		{
			new_num_nodes += m_num_nodes;
		}
	
		// and create the array for the rails	
		CRailNode* p_new_nodes = (CRailNode*)Mem::Malloc(new_num_nodes * sizeof(CRailNode));
	
		// add the old rails to the new data structure
		int next_node = 0;
		if (!arp.remove_old_rails)
		{
			// loop over the old nodes
			for (; next_node < m_num_nodes; next_node++)
			{
				CRailNode* pOldRailNode = &mp_nodes[next_node];
				CRailNode* pRailNode = &p_new_nodes[next_node];

				pRailNode->m_node = next_node;

				// calculate connectivity using pointer offsets
				pRailNode->m_pNextLink = (pOldRailNode->m_pNextLink
										  ? pOldRailNode->m_pNextLink - mp_nodes + p_new_nodes
										  : NULL);
				pRailNode->m_pPrevLink = (pOldRailNode->m_pPrevLink
										  ? pOldRailNode->m_pPrevLink - mp_nodes + p_new_nodes
										  : NULL);

				// copy in their state
				pRailNode->m_flags = pOldRailNode->m_flags;
				pRailNode->m_pos = pOldRailNode->m_pos;
				pRailNode->m_terrain_type = pOldRailNode->m_terrain_type; // debug colors for now pOldRailNode->m_terrain_type;
				pRailNode->m_BBMin = pOldRailNode->m_BBMin;
				pRailNode->m_BBMax = pOldRailNode->m_BBMax;
			} // END loop over old nodes
		} // END if retaining old rails
	
		// iterate over the nodes and add the new rails to the array; each time we hit a railset, we move to the head, then traverse the set, adding the
		// rails; not the most optimal algorithm but simple; we skip previously added rails and we move through the array
		for (int r = 0; r < arg.num_rails; r++)
		{
			// because we add whole railsets at once, we may already have added any given rail
			if (arg.p_rails[r].disabled) continue;

			// traverse to start of this rail's railset watching for a loop
			int s = r;
			while (arg.p_rails[s].endpoints[START].connection != -1)
			{
				s = arg.p_rails[s].endpoints[START].connection;
				if (s == r) break;
			}

			// traverse the railset, adding nodes as we go
			int starting_rail = s;
			CRailNode* p_starting_node = &p_new_nodes[next_node];
			int last_s;
			do {
				CRailNode* pRailNode = &p_new_nodes[next_node];
				pRailNode->m_node = next_node;

				if (arg.p_rails[s].endpoints[START].connection != -1)
				{
					pRailNode->m_pPrevLink = &p_new_nodes[next_node - 1];
				}
				else
				{
					pRailNode->m_pPrevLink = NULL;
				}

				// check for a loop
				if (arg.p_rails[s].endpoints[END].connection != starting_rail)
				{
					pRailNode->m_pNextLink = &p_new_nodes[next_node + 1];
				}
				else
				{
					pRailNode->m_pNextLink = p_starting_node;
				}

				pRailNode->m_flags = 0;
				pRailNode->SetActive(true);
				pRailNode->m_pos = arg.p_rails[s].endpoints[START].p;
				if (arg.p_rails[s].railset == -1)
					pRailNode->m_terrain_type = vTERRAIN_CONCSMOOTH; // red
				else
					pRailNode->m_terrain_type = vTERRAIN_METALSMOOTH; // blue
				Rail_ComputeBB(arg.p_rails[s].endpoints[START].p, arg.p_rails[s].endpoints[END].p, pRailNode->m_BBMin, pRailNode->m_BBMax);

				// mark as having been added
				arg.p_rails[s].disabled = true;
				next_node++;

				last_s = s;
				s = arg.p_rails[s].endpoints[END].connection;
			} while (s != -1 && s != starting_rail);

			// if not a loop
			if (s != starting_rail)
			{
				// add extra ending node of railset

				CRailNode* pRailNode = &p_new_nodes[next_node];
				pRailNode->m_node = next_node;
	
				pRailNode->m_pPrevLink = &p_new_nodes[next_node - 1];
				pRailNode->m_pNextLink = NULL;

				pRailNode->m_pos = arg.p_rails[last_s].endpoints[END].p;
	
				next_node++;
			}
		} // END final-rail-list generation scope
	
		// first reset the manager (this)
		// Note this invalidates mp_node_array (setting it to NULL)
		// so I had to patch up a couple of placed in skater.cpp
		// that were using the rail manager's node array
		Cleanup();
	
		mp_nodes = p_new_nodes;
 		m_num_nodes = new_num_nodes;
	
		printf("complete.\n");

	}

ABORT:

	delete	arg.p_rail_endpoint_list;

	delete	[] arg.p_railsets;	
	delete	[] arg.p_edges;
	delete  [] arg.p_rails;
	
	Mem::Manager::sHandle().PopContext();
	
	printf("-----------------------\n");
#endif	
}

}
			   

