/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Nx								 						**
**																			**
**	File name:		gel\collision\collide.cpp      							**
**																			**
**	Created by:		02/20/02	-	grj										**
**																			**
**	Description:															**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gel/collision/collision.h>
#include <gel/collision/colltridata.h>

#include <gfx/nx.h>
#include <gfx/nxflags.h>	// for face flag stuff
#include <gfx/debuggfx.h>

#include <sk/engine/feeler.h>


// For debugging rendering
#include <gfx/nxviewman.h>
#include <gfx/camera.h>

#ifdef	__PLAT_NGPS__	
#include <gfx/ngps/nx/line.h>
#include <gfx/ngps/p_nxsector.h>
#include <gfx/ngps/nx/geomnode.h>
#include <gfx/ngps/nx/render.h>
namespace	NxPs2
{
	bool TestSphereAgainstOccluders( Mth::Vector *p_center, float radius, uint32 meshes = 1 );
	bool	IsInFrame(Mth::Vector &center, float radius);
}

#endif

#ifdef	__PLAT_NGC__
#include <gfx/ngc/nx/scene.h>
#include <gfx/ngc/p_nxscene.h>
#endif		// __PLAT_NGC__


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/



namespace Nx
{


FaceIndex	CCollObjTriData::s_face_index_buffer[MAX_FACE_INDICIES];
FaceIndex	CCollObjTriData::s_seq_face_index_buffer[MAX_FACE_INDICIES] = { 0xFFFF }; // Set to uninitialized
uint		CCollObjTriData::s_num_face_indicies;

const uint	CCollObjTriData::s_max_face_per_leaf = 20;		// maximum number faces per leaf
const uint	CCollObjTriData::s_max_tree_levels = 7;			// maximum number of levels in a tree

#define USE_BSP_CLONE

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObjTriData::CCollObjTriData()
{
#ifdef __PLAT_NGC__
	mp_cloned_vert_pos = NULL;
#endif		// __PLAT_NGC__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObjTriData::~CCollObjTriData()
{
	// Clones are the only types that should make it here
	// Remove vertices
#ifdef __PLAT_NGC__
	if ( mp_cloned_vert_pos )
	{
		delete [] mp_cloned_vert_pos;
	}
#else
#ifdef FIXED_POINT_VERTICES
	if (mp_float_vert)
	{
		delete [] mp_float_vert;
	}
#else
	if (mp_vert_pos)
	{
		delete[] mp_vert_pos;
	}
#endif // FIXED_POINT_VERTICES
#endif		// __PLAT_NGC__
	if (mp_intensity)
	{
		delete[] mp_intensity;
	}

	// Remove faces
	if (mp_faces)
	{
		if (m_use_face_small)
		{
			delete[] mp_face_small;
		} else{
			delete[] mp_faces;
		}
	}

	DeleteBSPTree();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollObjTriData::InitCollObjTriData(CScene * p_scene, void *p_base_vert_addr, void *p_base_intensity_addr,
											void *p_base_face_addr, void *p_base_node_addr, void *p_base_face_idx_addr)
{
	// Set base addr
#ifdef __PLAT_NGC__
	NxNgc::sScene *p_engine_scene = ( static_cast<CNgcScene*>( p_scene ))->GetEngineScene();
	mp_raw_vert_pos = (NsVector*)p_engine_scene->mp_pos_pool;
	mp_intensity = (unsigned char *) ((int) p_base_vert_addr + (int)mp_intensity); 
	mp_cloned_vert_pos = NULL;
#else
#ifdef FIXED_POINT_VERTICES
	// Get indexes
	int start_vert_pos_offset = (int) mp_float_vert;
	int start_intensity_index = (int) mp_intensity;

	// Find first element
	mp_float_vert = (SFloatVert *) ((int) p_base_vert_addr + start_vert_pos_offset);
	mp_intensity = (uint8 *) p_base_intensity_addr;
	mp_intensity += start_intensity_index;
#else
	// Get indexes
	int start_vert_pos_offset = (int) mp_vert_pos;

	// Find first element
	mp_vert_pos = (Mth::Vector *) ((int) p_base_vert_addr + start_vert_pos_offset);

	mp_intensity = NULL;
#endif // FIXED_POINT_VERTICES
#endif		// __PLAT_NGC__

	mp_faces = (SFace *)((int) mp_faces + (int) p_base_face_addr);

	//Dbg_Message ( "Object has %d verts sizeof %d", m_num_verts, sizeof(SReadVertex));
	//Dbg_Message ( "Object has %d faces sizeof %d", m_num_faces, sizeof(SReadFace) );
#ifndef FIXED_POINT_VERTICES
	Dbg_Assert(!m_use_fixed_verts)
#endif

	// Init BSP tree
	mp_bsp_tree = (CCollBSPNode *)((int) mp_bsp_tree + (int) p_base_node_addr);
	s_init_tree(mp_bsp_tree, p_base_node_addr, p_base_face_idx_addr);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollBSPNode::CCollBSPNode()
{
	//m_node.m_split_axis = 0;
	m_node.m_split_point = 0; 		// This is so we know it is a node
	m_node.m_children.Init();
	//mp_greater_branch = NULL;
}

CCollBSPNode::~CCollBSPNode()
{
#ifdef USE_BSP_CLONE
	// Since the cloned BSP tree is allocated as one big buffer, these calls shouldn't
	// be necessary.
	return;

#else
	Dbg_MsgAssert(IsNode(), ("Called ~CCollBSPNode() on CCollBSPLeaf()"));

	Dbg_MsgAssert(0, ("Only cloned collision should get to the BSP destructor"));
#endif // USE_BSP_CLONE

	if (GetLessBranch())
	{
		delete GetLessBranch();
	}

	if (GetGreaterBranch())
	{
		delete GetGreaterBranch();
	}

	if (GetFaceIndexArray())
	{
		delete GetFaceIndexArray();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CCollBSPNode::SetSplitAxis(int axis)
{
	Dbg_Assert(IsNode());
	
	m_node.m_split_point = m_node.m_split_point & ~((1 << NUM_AXIS_BITS) - 1);		// Clear out old axis
	m_node.m_split_point = m_node.m_split_point | (axis & ((1 << NUM_AXIS_BITS) - 1));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Garrett: This is an unconventional way of cloning the data.  We are assuming that all the
// tree nodes are in one contiguous block of memory.  I normally wouldn't write code like
// this, but we need to avoid lots of small allocations.
CCollBSPNode * CCollBSPNode::clone(bool instance)
{
	Dbg_MsgAssert(!instance, ("CCollBSPNode::clone() with instances not implemented yet"));
	//Dbg_MsgAssert(IsNode(), ("Called CCollBSPNode::clone() on CCollBSPLeaf()"));

	// So that we don't make lots of little allocations, allocate the whole BSP array at once
	int num_nodes = count_bsp_nodes();
	int num_face_indices = count_bsp_face_indices();
	if (num_nodes > 0)
	{
		Dbg_MsgAssert(num_face_indices, ("Didn't find any face indices in the BSP tree"));
		FaceIndex *		p_orig_face_index_array = find_bsp_face_index_array_start();

		CCollBSPNode *	p_new_bsp_array = new CCollBSPNode[num_nodes];
		FaceIndex *		p_new_face_index_array = new FaceIndex[num_face_indices];

		// Just memcpy it first, since we know the source tree is in an array
		memcpy(p_new_bsp_array, this, num_nodes * sizeof(CCollBSPNode));
		memcpy(p_new_face_index_array, p_orig_face_index_array, num_face_indices * sizeof(FaceIndex));

#ifdef	__NOPT_ASSERT__
		// Look through new bsp array that still has the old face index pointers in it.
		for (int node_idx = 0; node_idx < num_nodes; node_idx++)
		{
			if (p_new_bsp_array[node_idx].IsLeaf())
			{
				Dbg_MsgAssert(p_orig_face_index_array <= p_new_bsp_array[node_idx].GetFaceIndexArray(), ("find_bsp_face_index_array_start() failed"));
			}
		}
#endif //	__NOPT_ASSERT__

		// Now adjust the pointers by finding the differences
		int node_address_diff = (int) p_new_bsp_array - (int) this;
		int face_address_diff = (int) p_new_face_index_array - (int) p_orig_face_index_array;
		for (int i = 0; i < num_nodes; i++)
		{
			if (p_new_bsp_array[i].IsNode())
			{
				// Adjust branch pointers
				p_new_bsp_array[i].m_node.m_children.SetBasePointer((CCollBSPNode *)((int) p_new_bsp_array[i].m_node.m_children.GetBasePointer() + node_address_diff));
			}
			else
			{
				// Adjust face index pointer
				p_new_bsp_array[i].m_leaf.mp_face_idx_array = (FaceIndex *) ((int) p_new_bsp_array[i].m_leaf.mp_face_idx_array + face_address_diff);
			}
		}

		return p_new_bsp_array;
	}
	else
	{
		return NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int				CCollBSPNode::count_bsp_nodes()
{
	if (IsLeaf())
	{
		return 1;
	}
	else
	{
		return 1 + GetLessBranch()->count_bsp_nodes() + GetGreaterBranch()->count_bsp_nodes();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int				CCollBSPNode::count_bsp_face_indices()
{
	if (IsLeaf())
	{
		return m_leaf.m_num_faces;
	}
	else
	{
		return GetLessBranch()->count_bsp_face_indices() + GetGreaterBranch()->count_bsp_face_indices();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

FaceIndex *		CCollBSPNode::find_bsp_face_index_array_start()
{
	if (IsLeaf())
	{
		return m_leaf.mp_face_idx_array;
	}
	else
	{
		FaceIndex *p_index1 = GetLessBranch()->find_bsp_face_index_array_start();
		FaceIndex *p_index2 = GetGreaterBranch()->find_bsp_face_index_array_start();

		if (p_index1 < p_index2)
			return p_index1;
		else
			return p_index2;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CCollBSPNode::CCollBSPChildren::SetLeftGreater(bool greater)
{
	 if (greater)
	 {
		 m_left_child_and_flags |= mLEFT_IS_GREATER;
	 }
	 else
	 {
		 m_left_child_and_flags &= ~mLEFT_IS_GREATER;
	 }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CCollBSPNode::translate(const Mth::Vector & delta_trans)
{
	// Translate node
	if (IsNode())
	{
		int axis = GetSplitAxis();

		SetFSplitPoint(GetFSplitPoint() + delta_trans[axis]);

		// Traverse the tree
		GetLessBranch()->translate(delta_trans);
		GetGreaterBranch()->translate(delta_trans);
	}

	// Leaf does nothing
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CCollBSPNode::rotate_y(const Mth::Vector & world_origin, Mth::ERot90 rot_y, bool root_node)
{
	// Take it to the origin
	if (root_node)
	{
		translate(-world_origin);
	}

	// Rotate node
	if (IsNode())
	{
		int axis = GetSplitAxis();

		if (axis != Y)		// Y won't change
		{
			int other_axis = (axis == X) ? Z : X;		// So we know what axis we potentially flip to

			switch (rot_y)
			{
			case Mth::ROT_0:
				break;

			case Mth::ROT_90:
				if (axis == X)
				{
					SetSplitPoint(-GetSplitPoint());
					m_node.m_children.SetLeftGreater(!m_node.m_children.IsLeftGreater());
				}
				SetSplitAxis(other_axis);
				break;

			case Mth::ROT_180:
				SetSplitPoint(-GetSplitPoint());
				m_node.m_children.SetLeftGreater(!m_node.m_children.IsLeftGreater());
				break;

			case Mth::ROT_270:
				if (axis == Z)
				{
					SetSplitPoint(-GetSplitPoint());
					m_node.m_children.SetLeftGreater(!m_node.m_children.IsLeftGreater());
				}
				SetSplitAxis(other_axis);
				break;

			default:
				Dbg_MsgAssert(0, ("CCollBSPNode::rotate_y() out of range: %d", rot_y));
				break;
			}
		}

		// Traverse the tree
		GetLessBranch()->rotate_y(world_origin, rot_y, false);
		GetGreaterBranch()->rotate_y(world_origin, rot_y, false);
	}
	// Leaf does nothing

	// Put it back
	if (root_node)
	{
		translate(world_origin);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CCollBSPNode::scale(const Mth::Vector & world_origin, const Mth::Vector & scale, bool root_node)
{
	// Take it to the origin
	if (root_node)
	{
		translate(-world_origin);
	}

	// Scale node
	if (IsNode())
	{
		int axis = GetSplitAxis();

		SetFSplitPoint(GetFSplitPoint() * scale[axis]);

		// Traverse the tree
		GetLessBranch()->scale(world_origin, scale, false);
		GetGreaterBranch()->scale(world_origin, scale, false);
	}
	// Leaf does nothing

	// Put it back
	if (root_node)
	{
		translate(world_origin);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollObjTriData::s_init_tree(CCollBSPNode *p_tree, void *p_base_node_addr, void *p_base_face_idx_addr)
{
	if (p_tree->IsLeaf())
	{
		// Set face index array pointer
		int face_idx = (int) p_tree->m_leaf.mp_face_idx_array;
		p_tree->m_leaf.mp_face_idx_array = (FaceIndex *) p_base_face_idx_addr;
		p_tree->m_leaf.mp_face_idx_array += face_idx;

		//Dbg_Assert(((int) p_leaf->mp_less_branch) == -1);
		//Dbg_Assert(((int) p_leaf->mp_greater_branch) == -1);

		//p_leaf->mp_less_branch = NULL;
		//p_leaf->mp_greater_branch = NULL;
	} else {
		Dbg_MsgAssert(p_tree->GetSplitAxis() < 3, ("BSP split axis is %d", p_tree->GetSplitAxis()));
		// Set branch pointers
		p_tree->m_node.m_children.SetBasePointer((CCollBSPNode *)((int) p_tree->m_node.m_children.GetBasePointer() + (int) p_base_node_addr));

		// And init branches
		s_init_tree(p_tree->GetLessBranch(), p_base_node_addr, p_base_face_idx_addr);
		s_init_tree(p_tree->GetGreaterBranch(), p_base_node_addr, p_base_face_idx_addr);
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollObjTriData::InitBSPTree()
{
	// Initialize sequence face index list, if not done yet
	if (s_seq_face_index_buffer[0] != 0)
	{
		for (int i = 0; i < MAX_FACE_INDICIES; i++)
		{
			s_seq_face_index_buffer[i] = i;
		}
	}

	Dbg_MsgAssert(m_num_faces < MAX_FACE_INDICIES, ("Too many polys in a collision sector: %d", m_num_faces));

	//Dbg_MsgAssert(0, ("Node size %d; Leaf size %d", sizeof(CCollBSPNode), sizeof(CCollBSPLeaf)));

	// Create the tree
   	// Don't use this since it is pip-ed
	//mp_bsp_tree = create_bsp_tree(m_bbox, s_seq_face_index_buffer, m_num_faces);

	return mp_bsp_tree != NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollObjTriData::DeleteBSPTree()
{
#ifndef USE_BSP_CLONE
   	// Don't use this since it is pip-ed
	return false;

	Dbg_MsgAssert(0, ("This should only be called on clones"));
#endif

	if (mp_bsp_tree)
	{
#ifndef USE_BSP_CLONE
		// Recursively delete
		delete mp_bsp_tree;
#else
		// Delete as array
		FaceIndex *p_face_index_array = mp_bsp_tree->find_bsp_face_index_array_start();
		if (p_face_index_array)
		{
			delete [] p_face_index_array;
		}
		delete [] mp_bsp_tree;
#endif
		mp_bsp_tree = NULL;

		return true;
	} else {
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollObjTriData::calc_split_faces(uint axis, float axis_distance, FaceIndex *p_face_indexes,
										  uint num_faces, uint & less_faces, uint & greater_faces,
										  FaceIndex *p_less_face_indexes, FaceIndex *p_greater_face_indexes)
{
	less_faces = greater_faces = 0;

	for (uint i = 0; i < num_faces; i++)
	{
		bool less = false, greater = false;

		// Check the face
		for (uint j = 0; j < 3; j++)
		{
			uint vidx = GetFaceVertIndex(i, j);
#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
			if (GetRawVertexPos(vidx)[axis] < axis_distance)
			{
				less = true;
			} else if (GetRawVertexPos(vidx)[axis] >= axis_distance)
			{
				greater = true;
			}
#else
			if (mp_vert_pos[vidx][axis] < axis_distance)
			{
				less = true;
			} else if (mp_vert_pos[vidx][axis] >= axis_distance)
			{
				greater = true;
			}
#endif
		}

		Dbg_Assert(less || greater);

		// Increment counts and possibly put in new array
		if (less)
		{
			if (p_less_face_indexes)
			{
				p_less_face_indexes[less_faces] = p_face_indexes[i];
			}
			less_faces++;
		}
		if (greater)
		{
			if (p_greater_face_indexes)
			{
				p_greater_face_indexes[greater_faces] = p_face_indexes[i];
			}
			greater_faces++;
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#if 0
CCollBSPLeaf * CCollObjTriData::create_bsp_leaf(FaceIndex *p_face_indexes, uint num_faces)
{
	CCollBSPLeaf *p_bsp_leaf = new CCollBSPLeaf;
	p_bsp_leaf->m_split_axis = -1;
	p_bsp_leaf->mp_less_branch = NULL;
	p_bsp_leaf->mp_greater_branch = NULL;

	// Make new array in BottomUp memory
	p_bsp_leaf->m_num_faces = num_faces;
	p_bsp_leaf->mp_face_idx_array = new FaceIndex[num_faces];
	for (uint i = 0; i < num_faces; i++)
	{
		p_bsp_leaf->mp_face_idx_array[i] = p_face_indexes[i];
	}

	return p_bsp_leaf;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollBSPNode * CCollObjTriData::create_bsp_tree(const Mth::CBBox & bbox, FaceIndex *p_face_indexes, uint num_faces, uint level)
{
	if ((num_faces <= s_max_face_per_leaf) || (level == s_max_tree_levels)) // Check if this should be a leaf
	{
		return create_bsp_leaf(p_face_indexes, num_faces);
	} else {																// Create Node

		// Find initial splits on the three axis
		Mth::Vector mid_width((bbox.GetMax() - bbox.GetMin()) * 0.5f);
		Mth::Vector mid_split(mid_width + bbox.GetMin());

		// Find the weighting of the three potential splits
		uint less_faces[3], greater_faces[3];
		calc_split_faces(X, mid_split[X], p_face_indexes, num_faces, less_faces[X], greater_faces[X]);
		calc_split_faces(Y, mid_split[Y], p_face_indexes, num_faces, less_faces[Y], greater_faces[Y]);
		calc_split_faces(Z, mid_split[Z], p_face_indexes, num_faces, less_faces[Z], greater_faces[Z]);

		// Figure out best split
		int best_axis = -1;
		float best_diff = -1;
		const int duplicate_threshold = (num_faces * 7) / 10;	// tunable
		for (uint axis = X; axis <= Z; axis++)
		{
			float new_diff = (float)fabs((float)(less_faces[axis] - greater_faces[axis]));
			int duplicates = less_faces[axis] + greater_faces[axis] - num_faces;

			if (duplicates >= duplicate_threshold)
				continue;

			new_diff += duplicates;				// tunable
			new_diff /= mid_width[axis];		// tunable

			if ((best_axis < 0) || (new_diff < best_diff))
			{
				best_axis = axis;
				best_diff = new_diff;
			}
		}

		if (best_axis < 0)			// Couldn't make a good split, give up
		{
			return create_bsp_leaf(p_face_indexes, num_faces);
		}

		// We need to allocate temp arrays
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

		// Allocate new temp arrays for the face indexes
		FaceIndex *p_less_face_indexes = new FaceIndex[less_faces[best_axis]];
		FaceIndex *p_greater_face_indexes = new FaceIndex[greater_faces[best_axis]];

		Mem::Manager::sHandle().PopContext();

		// Now fill in the array
		calc_split_faces(best_axis, mid_split[best_axis], p_face_indexes, num_faces, 
						 less_faces[best_axis], greater_faces[best_axis],
						 p_less_face_indexes, p_greater_face_indexes);

		// And the new bboxes
		Mth::CBBox less_bbox(bbox), greater_bbox(bbox);
		Mth::Vector less_max = less_bbox.GetMax();
		Mth::Vector greater_min = greater_bbox.GetMin();
		less_max[best_axis] = mid_split[best_axis];
		greater_min[best_axis] = mid_split[best_axis];
		less_bbox.SetMax(less_max);
		greater_bbox.SetMin(greater_min);
	
		// And now calculate the branches
		CCollBSPNode *p_bsp_tree = new CCollBSPNode;
		p_bsp_tree->m_split_axis = best_axis;
		p_bsp_tree->m_split_point = mid_split[best_axis];
		p_bsp_tree->mp_less_branch = create_bsp_tree(less_bbox, p_less_face_indexes, less_faces[best_axis], level + 1);
		p_bsp_tree->mp_greater_branch = create_bsp_tree(greater_bbox, p_greater_face_indexes, greater_faces[best_axis], level + 1);

		// Free temp arrays
		delete p_less_face_indexes;
		delete p_greater_face_indexes;

		return p_bsp_tree;
	}
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CCollObjTriData::find_faces(CCollBSPNode *p_bsp_node, const Mth::CBBox & bbox)
{
	int axis = p_bsp_node->GetSplitAxis();

	if (axis == 3)	// Leaf
	{
		//CCollBSPLeaf *p_bsp_leaf = static_cast<CCollBSPLeaf *>(p_bsp_node);		// We are certain that it is this type
		uint num_faces = p_bsp_node->m_leaf.m_num_faces;
		FaceIndex *p_dest_buffer = &(s_face_index_buffer[s_num_face_indicies]);
		FaceIndex *p_src_buffer = p_bsp_node->m_leaf.mp_face_idx_array;

		for (uint i = 0; i < num_faces; i++)
		{
			//s_face_index_buffer[s_num_face_indicies++] = p_bsp_leaf->mp_face_idx_array[i];
			*(p_dest_buffer++) = *(p_src_buffer++);
		}

		s_num_face_indicies += num_faces;
		Dbg_Assert(s_num_face_indicies < MAX_FACE_INDICIES);
	} else {							// Node
		float f_split_point = p_bsp_node->GetFSplitPoint();

		if (bbox.GetMin()[axis] < f_split_point)
		{
			find_faces(p_bsp_node->GetLessBranch(), bbox);
		}

		if (bbox.GetMax()[axis] >= f_split_point)
		{
			find_faces(p_bsp_node->GetGreaterBranch(), bbox);
		}
	}
}

// Calculate the normal of a face
// this is rather expensive if doing a lot of processing, so don't call
// this function more than you need to.
const Mth::Vector CCollObjTriData::GetFaceNormal(int face_idx) const
{
		Mth::Vector v0 = GetRawVertexPos(GetFaceVertIndex(face_idx,0));
		Mth::Vector v1 = GetRawVertexPos(GetFaceVertIndex(face_idx,1));
		Mth::Vector v2 = GetRawVertexPos(GetFaceVertIndex(face_idx,2));

		// Find normal
		Mth::Vector vTmp1(v1 - v0);
		Mth::Vector vTmp2(v2 - v0);
		Mth::Vector normal = Mth::CrossProduct(vTmp1, vTmp2);
		normal.Normalize();
		return normal;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

FaceIndex *			CCollObjTriData::FindIntersectingFaces(const Mth::CBBox & line_bbox, uint & num_faces)
{
	// Make sure we have a tree
	if (!mp_bsp_tree)
	{
		num_faces = m_num_faces;
		Dbg_Assert(num_faces < MAX_FACE_INDICIES);
		return s_seq_face_index_buffer;
	}

	// Search tree
	s_num_face_indicies = 0;
	find_faces(mp_bsp_tree, line_bbox);

	num_faces = s_num_face_indicies;

	#if 0	
	// see if there are any duplicates....
	int dups = 0;
	if (num_faces)
	{
		for (uint i = 0;i<num_faces-1;i++)
		{
			FaceIndex x = s_face_index_buffer[i];
			for (uint j = i+1; j<num_faces;j++)
			{
				if (s_face_index_buffer[j] == x)
				{
					dups++;
				}
			}
		}
	}
	printf ("%d/%d/%d\n",dups,num_faces,m_num_faces);
	#endif
	
	return s_face_index_buffer;
}

//************************************************************************************
//
// End of BSP code
//
//************************************************************************************


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObjTriData *	CCollObjTriData::Clone(bool instance, bool skip_no_verts)
{
	// Don't clone when there is no geometry and skip_no_verts is set
	if (skip_no_verts && (m_num_verts == 0))
	{
		return NULL;
	}

	int intensity_array_size;
	CCollObjTriData *m_new_coll = new CCollObjTriData(*this);

	Dbg_MsgAssert(!instance, ("CCollObjTriData::Clone() with instances not implemented yet"));
	Dbg_MsgAssert(m_num_verts, ("CCollObjTriData::Clone() with no geometry not implemented yet"));
	Dbg_Assert(m_num_faces);

#ifdef __PLAT_NGC__
//	m_new_coll->mp_vert_pos	= (NsVector*)new char[CCollObjTriData::GetVertElemSize()*m_num_verts];
	m_new_coll->mp_raw_vert_pos	= mp_raw_vert_pos;
	intensity_array_size = m_num_faces * 3;
#else
	if (m_use_fixed_verts)
	{
		m_new_coll->mp_fixed_vert = new SFixedVert[m_num_verts];
	}
	else
	{
#ifdef FIXED_POINT_VERTICES
		m_new_coll->mp_float_vert = new SFloatVert[m_num_verts];
#else
		m_new_coll->mp_vert_pos	= new Mth::Vector[m_num_verts];
#endif
	}
	intensity_array_size = m_num_verts;
#endif		// __PLAT_NGC__
	if (m_use_face_small)
	{
		m_new_coll->mp_face_small = new SFaceSmall[m_num_faces];
	} else {
		m_new_coll->mp_faces = new SFace[m_num_faces];
	}

#ifdef FIXED_POINT_VERTICES
	if (m_new_coll->mp_float_vert && m_new_coll->mp_faces)
#else
	if (m_new_coll->mp_raw_vert_pos && m_new_coll->mp_faces)
#endif
	{
#ifdef __PLAT_NGC__
		memcpy(m_new_coll->mp_faces,		mp_faces		, m_num_faces * sizeof(SFace));
#else
		if (m_use_fixed_verts)
		{
			memcpy(m_new_coll->mp_fixed_vert,	mp_fixed_vert	, m_num_verts * CCollObjTriData::GetVertSmallElemSize());
		}
		else
		{
#ifdef FIXED_POINT_VERTICES
			memcpy(m_new_coll->mp_float_vert,	mp_float_vert	, m_num_verts * CCollObjTriData::GetVertElemSize());
#else
			memcpy(m_new_coll->mp_vert_pos,		mp_vert_pos		, m_num_verts * CCollObjTriData::GetVertElemSize());
#endif
		}
		if (m_use_face_small)
		{
			memcpy(m_new_coll->mp_face_small,	mp_face_small	, m_num_faces * sizeof(SFaceSmall));
		} else {
			memcpy(m_new_coll->mp_faces,		mp_faces		, m_num_faces * sizeof(SFace));
		}
#endif		// __PLAT_NGC__
	} else {
		Dbg_Error("Can't allocate new collision data");
		return NULL;
	}

	// Copy intensity array if there is one
	if (mp_intensity)
	{
		m_new_coll->mp_intensity = new uint8[intensity_array_size];
		memcpy(m_new_coll->mp_intensity, mp_intensity , intensity_array_size * sizeof(uint8));
	}

	// Set BSP tree to NULL for now until we come up with a method to copy and translate
#ifdef USE_BSP_CLONE
	m_new_coll->mp_bsp_tree = mp_bsp_tree->clone(instance);
#else
	m_new_coll->mp_bsp_tree = NULL;
#endif // USE_BSP_CLONE

	return m_new_coll;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::Translate(const Mth::Vector & delta_pos)
{
	Mth::Vector min_point, max_point;
	int i;


	// Translate BSP tree
	if (mp_bsp_tree)
	{
		//printf ("WARNING:  moving colision geometry that contains a BSP tree.  *** CLEARING ***\n");
		//mp_bsp_tree = NULL;
		mp_bsp_tree->translate(delta_pos);
	}

	#ifdef __PLAT_NGC__
	// Get Verts
	Mth::Vector *p_float_verts = NULL;
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	p_float_verts = new Mth::Vector[m_num_verts];
	Mem::Manager::sHandle().PopContext();
	get_float_array_from_data(p_float_verts);
	
	// Translate
	for ( i = 0; i < m_num_verts; i++ )
	{
		p_float_verts[i] += delta_pos;
	}

	// Put Verts
	put_float_array_into_data(p_float_verts);
	delete [] p_float_verts;


//	if ( !mp_offset ) mp_offset = new Mth::Vector;
//	mp_offset[0][X] += delta_pos[X];
//	mp_offset[0][Y] += delta_pos[Y];
//	mp_offset[0][Z] += delta_pos[Z];
	#else
	// Pos (don't touch W since the color is there)
	if (!m_use_fixed_verts)
	{
		for (i = 0; i < m_num_verts; i++)
		{
	#ifdef FIXED_POINT_VERTICES
			mp_float_vert[i].m_pos[X] += delta_pos[X];
			mp_float_vert[i].m_pos[Y] += delta_pos[Y];
			mp_float_vert[i].m_pos[Z] += delta_pos[Z];
	#else
			mp_vert_pos[i][X] += delta_pos[X];
			mp_vert_pos[i][Y] += delta_pos[Y];
			mp_vert_pos[i][Z] += delta_pos[Z];
	#endif // FIXED_POINT_VERTICES
		}
	}
	#endif		// __PLAT_NGC__

	// BBox
	min_point = m_bbox.GetMin() + delta_pos;
	max_point = m_bbox.GetMax() + delta_pos;
	m_bbox.Set(min_point, max_point);

	//Dbg_Message("Min BBox (%f, %f, %f)", m_bbox.GetMin()[X], m_bbox.GetMin()[Y], m_bbox.GetMin()[Z]);
	//Dbg_Message("Max BBox (%f, %f, %f)", m_bbox.GetMax()[X], m_bbox.GetMax()[Y], m_bbox.GetMax()[Z]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::RotateY(const Mth::Vector & world_origin, Mth::ERot90 rot_y)
{
	Mth::Vector min_point, max_point;
	int i;

	// Put object at origin
	Translate(-world_origin);

	// Rotate BSP tree
	if (mp_bsp_tree)
	{
		mp_bsp_tree->rotate_y(world_origin, rot_y, false);		// Don't allow it to do another translation
	}

	// Convert to floating point, if fixed point data
	Mth::Vector *p_float_verts = NULL;
	#ifndef __PLAT_NGC__
	if (m_use_fixed_verts)
	#endif		// __PLAT_NGC__
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
		p_float_verts = new Mth::Vector[m_num_verts];
		Mem::Manager::sHandle().PopContext();

		get_float_array_from_data(p_float_verts);
	}

	// Rotate
	for (i = 0; i < m_num_verts; i++)
	{

		if (p_float_verts)
		{
			p_float_verts[i].RotateY90(rot_y);
		}
		#ifndef __PLAT_NGC__
		else
		{
		#ifdef FIXED_POINT_VERTICES
			Mth::Vector pos(Mth::Vector::NO_INIT);
			GetRawVertexPos(i, pos);
			pos.RotateY90(rot_y);
			mp_float_vert[i].m_pos[X] = pos[X];
			mp_float_vert[i].m_pos[Y] = pos[Y];
			mp_float_vert[i].m_pos[Z] = pos[Z];
		#else
			mp_vert_pos[i].RotateY90(rot_y);
		#endif
		}
		#endif		// __PLAT_NGC__
	}

	// BBox rotate
	min_point = m_bbox.GetMin();
	max_point = m_bbox.GetMax();
	min_point.RotateY90(rot_y);
	max_point.RotateY90(rot_y);
	m_bbox.Reset();
	m_bbox.AddPoint(min_point);
	m_bbox.AddPoint(max_point);

	// Convert back to fixed point, if necessary
	if (p_float_verts)
	{
		put_float_array_into_data(p_float_verts);
		delete [] p_float_verts;
	}

	// Put object back
	Translate(world_origin);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::Scale(const Mth::Vector & world_origin, const Mth::Vector & scale)
{
	// Put object at origin
	Translate(-world_origin);

	// Scale BSP tree
	if (mp_bsp_tree)
	{
		mp_bsp_tree->scale(world_origin, scale, false);		// Don't allow it to do another translation
	}

	// Convert to floating point, if fixed point data
	Mth::Vector *p_float_verts = NULL;
	#ifndef __PLAT_NGC__
	if (m_use_fixed_verts)
	#endif		// __PLAT_NGC__
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
		p_float_verts = new Mth::Vector[m_num_verts];
		Mem::Manager::sHandle().PopContext();

		get_float_array_from_data(p_float_verts);
	}

	// Pos (don't touch W since the color is there)
	for (int i = 0; i < m_num_verts; i++)
	{
		if (p_float_verts)
		{
			p_float_verts[i][X] *= scale[X];
			p_float_verts[i][Y] *= scale[Y];
			p_float_verts[i][Z] *= scale[Z];
		}
		#ifndef __PLAT_NGC__
		else
		{
		#ifdef FIXED_POINT_VERTICES
			mp_float_vert[i].m_pos[X] *= scale[X];
			mp_float_vert[i].m_pos[Y] *= scale[Y];
			mp_float_vert[i].m_pos[Z] *= scale[Z];
		#else
			mp_vert_pos[i][X] *= scale[X];
			mp_vert_pos[i][Y] *= scale[Y];
			mp_vert_pos[i][Z] *= scale[Z];
		#endif
		}
		#endif		// __PLAT_NGC__
	}

	// BBox
	Mth::Vector min_point(m_bbox.GetMin()), max_point(m_bbox.GetMax());

	min_point[X] *= scale[X];
	min_point[Y] *= scale[Y];
	min_point[Z] *= scale[Z];

	max_point[X] *= scale[X];
	max_point[Y] *= scale[Y];
	max_point[Z] *= scale[Z];

	m_bbox.Set(min_point, max_point);

	// Convert back to fixed point, if necessary
	if (p_float_verts)
	{
		put_float_array_into_data(p_float_verts);
		delete [] p_float_verts;
	}

	// Put object back
	Translate(world_origin);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::SetRawVertexPos(int vert_idx, const Mth::Vector & pos)
{
	Dbg_MsgAssert(mp_bsp_tree == NULL, ("Cannot change a vertex within a BSP Tree"));

	set_vertex_pos(vert_idx, pos);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::set_vertex_pos(int vert_idx, const Mth::Vector & pos)
{
#ifndef __PLAT_NGC__	// Since they are shared with the render data
	if (m_use_fixed_verts)
	{
		Mth::Vector rel_pos(pos - m_bbox.GetMin());

		Dbg_MsgAssert((rel_pos[X] >= 0.0f) && (rel_pos[Y] >= 0.0f) && (rel_pos[Z] >= 0.0f), ("Adding a vert that is outside of bounding box"));

		mp_fixed_vert[vert_idx].m_pos[X] = (uint16) (rel_pos[X] * COLLISION_SUB_INCH_PRECISION);
		mp_fixed_vert[vert_idx].m_pos[Y] = (uint16) (rel_pos[Y] * COLLISION_SUB_INCH_PRECISION);
		mp_fixed_vert[vert_idx].m_pos[Z] = (uint16) (rel_pos[Z] * COLLISION_SUB_INCH_PRECISION);
	}
	else
	{
#ifdef FIXED_POINT_VERTICES
		mp_float_vert[vert_idx].m_pos[X] = pos[X];
		mp_float_vert[vert_idx].m_pos[Y] = pos[Y];
		mp_float_vert[vert_idx].m_pos[Z] = pos[Z];
#else
		mp_vert_pos[vert_idx][X] = pos[X];
		mp_vert_pos[vert_idx][Y] = pos[Y];
		mp_vert_pos[vert_idx][Z] = pos[Z];
#endif
	}
#endif		// __PLAT_NGC__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::GetRawVertices(Mth::Vector *p_vert_array) const
{
	get_float_array_from_data(p_vert_array);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::SetRawVertices(const Mth::Vector *p_vert_array)
{
	// Must set the bounding box first in case data is in fixed point
	m_bbox.Reset();
	for (int i = 0; i < m_num_verts; i++)
	{
		m_bbox.AddPoint(p_vert_array[i]);
	}

	put_float_array_into_data(p_vert_array);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define MAX_USED 80000

void	CCollObjTriData::get_float_array_from_data(Mth::Vector *p_float_array) const
{
#ifdef __PLAT_NGC__
	if ( mp_cloned_vert_pos )
	{
		// Already cloned, just copy.
		for ( int i = 0; i < m_num_verts; i++ )
		{
			p_float_array[i][X] = mp_cloned_vert_pos[i].x;
			p_float_array[i][Y] = mp_cloned_vert_pos[i].y;
			p_float_array[i][Z] = mp_cloned_vert_pos[i].z;
			p_float_array[i][W] = 0.0f;
		}
	}
	else
	{
		// We need to pull the verts out of the main render vert list.
		int lp;
		int lp2;
		int index;
		unsigned char used[(MAX_USED/8)];

		// Build a bit list of used verts.
		for ( lp = 0; lp < (MAX_USED/8); lp++ ) used[lp] = 0;

		for ( lp = 0; lp < m_num_faces; lp++ )
		{
			index = mp_faces[lp].m_vertex_index[0];
			used[(index>>3)] |= (1<<(index&7));
			index = mp_faces[lp].m_vertex_index[1];
			used[(index>>3)] |= (1<<(index&7));
			index = mp_faces[lp].m_vertex_index[2];
			used[(index>>3)] |= (1<<(index&7));
		}

		// Copy verts out in order.
		int num_copied = 0;
		for ( lp = 0; lp < (MAX_USED/8); lp++ )
		{
			if ( used[lp] )
			{
				for ( lp2 = 0; lp2 < 8; lp2++ )
				{
					if ( used[lp] & (1<<lp2) )
					{
						index = (lp<<3) + lp2;
						p_float_array[num_copied] = GetRawVertexPos(index);
						num_copied++;
					}
				}
			}
		}
	}
#else
	for (int i = 0; i < m_num_verts; i++)
	{
		p_float_array[i] = GetRawVertexPos(i);
	}
#endif		// __PLAT_NGC__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::put_float_array_into_data(const Mth::Vector *p_float_array)
{
#ifdef __PLAT_NGC__
	// Need to change mp_mod to a modified collision vert pointer and alloc new space here.
	// the header getpos functions need to check that & pull from there if necessary.
	// Also need to remap the face array when we do this.
	// Ugh.



	if ( mp_cloned_vert_pos )
	{
		// Already cloned, just copy.
		for ( int i = 0; i < m_num_verts; i++ )
		{
			mp_cloned_vert_pos[i].x = p_float_array[i][X];
			mp_cloned_vert_pos[i].y = p_float_array[i][Y];
			mp_cloned_vert_pos[i].z = p_float_array[i][Z];
		}
	}
	else
	{
		// Allocate space for verts & copy over.
		mp_cloned_vert_pos = (NsVector*)new float[m_num_verts*3];

		for ( int i = 0; i < m_num_verts; i++ )
		{
			mp_cloned_vert_pos[i].x = p_float_array[i][X];
			mp_cloned_vert_pos[i].y = p_float_array[i][Y];
			mp_cloned_vert_pos[i].z = p_float_array[i][Z];
		}

		// Need to remap existing face map.
		int lp;
		int lp2;
		int index;
		unsigned char used[(MAX_USED/8)];

		// Build a bit list of used verts.
		for ( lp = 0; lp < (MAX_USED/8); lp++ ) used[lp] = 0;

		for ( lp = 0; lp < m_num_faces; lp++ )
		{
			index = mp_faces[lp].m_vertex_index[0];
			used[(index>>3)] |= (1<<(index&7));
			index = mp_faces[lp].m_vertex_index[1];
			used[(index>>3)] |= (1<<(index&7));
			index = mp_faces[lp].m_vertex_index[2];
			used[(index>>3)] |= (1<<(index&7));
		}

		// Remap faces.
		int num_copied = 0;
		for ( lp = 0; lp < (MAX_USED/8); lp++ )
		{
			if ( used[lp] )
			{
				for ( lp2 = 0; lp2 < 8; lp2++ )
				{
					if ( used[lp] & (1<<lp2) )
					{
						index = (lp<<3) + lp2;

						// See if any face indices match this one & remap if so.
						for ( int f = 0; f < m_num_faces; f++ )
						{
							for ( int v = 0; v < 3; v++ )
							{
								if ( mp_faces[f].m_vertex_index[v] == index )
								{
									mp_faces[f].m_vertex_index[v] = num_copied;
								}
							}
						}
						num_copied++;
					}
				}
			}
		}
	}
#else
	for (int i = 0; i < m_num_verts; i++)
	{
		set_vertex_pos(i, p_float_array[i]);
	}
#endif		// __PLAT_NGC__
}

// This is called once when a scene is first loaded, from ScriptParseNodeArray
// and only if the OCCLUDER flag is set in the object's node
// here we loop over the polgons in the object
// and create a single occluder polygon
// which is added to the world
//
// (these will need to be deleted when the scene is unloaded)
void CCollObjTriData::ProcessOcclusion( void )
{
	// Scan through and find pairs of triangles that share 2 verts - i.e. that form quads.
	for( int fidx0 = 0; fidx0 < m_num_faces - 1; ++fidx0 )
	{
		SFaceInfo *p_face0 = get_face_info(fidx0);

		// Check we haven't already used this face.
		if( p_face0->m_flags & ( 1 << 31 ))
		{
			continue;
		}
		
		Mth::Vector	v[4];
		v[0] = GetRawVertexPos(GetFaceVertIndex(fidx0, 0));
		v[1] = GetRawVertexPos(GetFaceVertIndex(fidx0, 1));
		v[2] = GetRawVertexPos(GetFaceVertIndex(fidx0, 2));
//		printf( "Occlusion Triangle %d\n(%f,%f,%f)\n(%f,%f,%f)\n(%f,%f,%f)\n",fidx0,v[0][X],v[0][Y],v[0][Z],v[1][X],v[1][Y],v[1][Z],v[2][X],v[2][Y],v[2][Z] );

		for( int fidx1 = fidx0 + 1; fidx1 < m_num_faces; ++fidx1 )
		{
			SFaceInfo *p_face1 = get_face_info(fidx1);

			// Check we haven't already used this face.
			if( p_face1->m_flags & ( 1 << 31 ))
			{
				continue;
			}

			v[0] = GetRawVertexPos(GetFaceVertIndex(fidx1, 0));
			v[1] = GetRawVertexPos(GetFaceVertIndex(fidx1, 1));
			v[2] = GetRawVertexPos(GetFaceVertIndex(fidx1, 2));
//			printf( "Occlusion Triangle %d\n(%f,%f,%f)\n(%f,%f,%f)\n(%f,%f,%f)\n",fidx1,v[0][X],v[0][Y],v[0][Z],v[1][X],v[1][Y],v[1][Z],v[2][X],v[2][Y],v[2][Z] );

			int indices_matched0[3]	= { -1, -1, -1 };
			int indices_matched1[3]	= { -1, -1, -1 };
			int num_indices_matched	= 0;

			for( int i = 0; i < 3; ++i )
			{
				if( GetFaceVertIndex(fidx0, i) == GetFaceVertIndex(fidx1, 0) )
				{
					indices_matched0[i] = GetFaceVertIndex(fidx0, i);
					indices_matched1[0] = GetFaceVertIndex(fidx1, 0);
					++num_indices_matched;
				}
				else if( GetFaceVertIndex(fidx0, i) == GetFaceVertIndex(fidx1, 1) )
				{
					indices_matched0[i] = GetFaceVertIndex(fidx0, i);
					indices_matched1[1] = GetFaceVertIndex(fidx1, 1);
					++num_indices_matched;
				}
				else if( GetFaceVertIndex(fidx0, i) == GetFaceVertIndex(fidx1, 2) )
				{
					indices_matched0[i] = GetFaceVertIndex(fidx0, i);
					indices_matched1[2] = GetFaceVertIndex(fidx1, 2);
					++num_indices_matched;
				}
			}

			// Three indices matched is just plain wrong.
			Dbg_Assert( num_indices_matched < 3 );

			// Two indices matched is just what we want.
			if( num_indices_matched == 2 )
			{
				// Make sure that the 2 tris lie in the same plane, which is no longer necessarily the case since they could be
				// tris which adjoin at a right angle.
				v[0]				= GetRawVertexPos(GetFaceVertIndex( fidx0, 0 ));
				v[1]				= GetRawVertexPos(GetFaceVertIndex( fidx0, 1 ));
				v[2]				= GetRawVertexPos(GetFaceVertIndex( fidx0, 2 ));
				Mth::Vector norm0	= Mth::CrossProduct( v[1] - v[0], v[2] - v[0] );
				v[0]				= GetRawVertexPos(GetFaceVertIndex( fidx1, 0 ));
				v[1]				= GetRawVertexPos(GetFaceVertIndex( fidx1, 1 ));
				v[2]				= GetRawVertexPos(GetFaceVertIndex( fidx1, 2 ));
				Mth::Vector norm1	= Mth::CrossProduct( v[1] - v[0], v[2] - v[0] );
				norm0.Normalize();
				norm1.Normalize();
				
				if(( fabs( norm1[X]-norm0[X]) <= 0.05f ) && ( fabs( norm1[Y]-norm0[Y]) <= 0.05f ) && ( fabs( norm1[Z]-norm0[Z]) <= 0.05f ))
				{
								
					// These two tris are coplanar.
				
					// Get the index from tri1 that isn't shared with tri0.
					int unshared_tri1_index = -1;
					for( int i = 0; i < 3; ++i )
					{
						if( indices_matched1[i] == -1 )
						{
							unshared_tri1_index = GetFaceVertIndex(fidx1, i);
							break;
						}
					}
					Dbg_Assert( unshared_tri1_index >= 0 );
				
					// Fill in the missing 2 verts, one from each tri. What is important here is which side of triangle0 that
					// that triangle1 shares, since this will determine the overall order of the quad abcd.
					if(( indices_matched0[0] >= 0 ) && ( indices_matched0[1] >= 0 ))
					{
						// They share side 0->1. Thus the quad will be t0(0), t1(unshared), t0(1), t0(2).
						v[0] = GetRawVertexPos(GetFaceVertIndex(fidx0, 0));
						v[1] = GetRawVertexPos(unshared_tri1_index);
						v[2] = GetRawVertexPos(GetFaceVertIndex(fidx0, 1));
						v[3] = GetRawVertexPos(GetFaceVertIndex(fidx0, 2));
					}
					else if(( indices_matched0[1] >= 0 ) && ( indices_matched0[2] >= 0 ))
					{
						// They share side 1->2. Thus the quad will be t0(0), t0(1), t1(unshared), t0(2).
						v[0] = GetRawVertexPos(GetFaceVertIndex(fidx0, 0));
						v[1] = GetRawVertexPos(GetFaceVertIndex(fidx0, 1));
						v[2] = GetRawVertexPos(unshared_tri1_index);
						v[3] = GetRawVertexPos(GetFaceVertIndex(fidx0, 2));
					}
					else if(( indices_matched0[0] >= 0 ) && ( indices_matched0[2] >= 0 ))
					{
						// They share side 2->0. Thus the quad will be t0(0), t0(1), t0(2), t1(unshared).
						v[0] = GetRawVertexPos(GetFaceVertIndex(fidx0, 0));
						v[1] = GetRawVertexPos(GetFaceVertIndex(fidx0, 1));
						v[2] = GetRawVertexPos(GetFaceVertIndex(fidx0, 2));
						v[3] = GetRawVertexPos(unshared_tri1_index);
					}
					else
					{
						Dbg_Assert( 0 );
					}

//					printf( "Occlusion Quad\n(%f,%f,%f)\n(%f,%f,%f)\n(%f,%f,%f)\n(%f,%f,%f)\n",v[0][X],v[0][Y],v[0][Z],v[1][X],v[1][Y],v[1][Z],v[2][X],v[2][Y],v[2][Z],v[3][X],v[3][Y],v[3][Z] );

					// Register this quad with the engine.
					Nx::CEngine::sAddOcclusionPoly( 4, v, m_checksum );
				
					// Flag the two tris as having been used.
					Dbg_Assert(( p_face0->m_flags & ( 1 << 31 )) == 0 );
					Dbg_Assert(( p_face1->m_flags & ( 1 << 31 )) == 0 );
					p_face0->m_flags |= ( 1 << 31 );
					p_face1->m_flags |= ( 1 << 31 );
				
					// Break out of this inner loop now since we have found a pair.
					break;
				}
			}
		}
	}

	// Scan through and clear 'used' flag on faces.
	for( int fidx0 = 0; fidx0 < m_num_faces; ++fidx0 )
	{
		SFaceInfo *p_face0 = get_face_info(fidx0);
		p_face0->m_flags &= ~( 1 << 31 );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef	__DEBUG_CODE__

void	CCollObjTriData::DebugRender(uint32 ignore_1, uint32 ignore_0)
{
#ifdef	__NOPT_ASSERT__
	static Mth::Matrix ident_matrix(0.0f, 0.0f, 0.0f);	// static so it doesn't waste time in the constuctor

	// simple culling based on the world bbox
	Mth::Vector mid = (GetBBox().GetMax() + GetBBox().GetMin())/2.0f;
	float	radius = (mid - GetBBox().GetMin()).Length();

	if (Nx::CEngine::GetWireframeMode() == 6)	 // 6 = occlusion	  
	{
		// Rendering as 2d, to demontrate occlusion
		// we try to get the visibility flag from the engine

#if	0		//def	__PLAT_NGPS__	
	bool	drawn = true;

	Nx::CPs2Sector* p_sector = (Nx::CPs2Sector*)Nx::CEngine::sGetSector(m_checksum);
	if (p_sector)
	{
	
		NxPs2::CGeomNode *p_node = (p_sector->GetEngineObject());
		if (p_node)
		{
			drawn = p_node->WasRendered();		
		}
	}


//
	if (drawn)
#else
		if ( Nx::CEngine::sIsVisible(mid,radius))
#endif	
		{
			Nx::CSector* p_sector = Nx::CEngine::sGetSector(m_checksum); // SLOWWWWWWWWWWWWW
			if (p_sector && !p_sector->GetVisibility())
			{
				DebugRender2D(ignore_1, ignore_0,0x000080);   			// red = hidden
			}
			else
			{
				DebugRender2D(ignore_1, ignore_0,0x808080);   			// grey = visible
			}
		}
		else
		{
			#ifdef	__PLAT_NGPS__
			// Mick:  Should be abel to use the actual Occluded bits here....
			if (NxPs2::IsInFrame( mid, radius ) && NxPs2::TestSphereAgainstOccluders(&mid,radius,0))
				DebugRender2D(ignore_1, ignore_0,0x40c040);	   // green = occluded 
			else
			#endif
			{
				DebugRender2DBBox(ignore_1, ignore_0,0x003030);		// dk yellow box and 
				DebugRender2DOct(ignore_1, ignore_0,0x003000);		// dark green ocotogon off camera
			}
		}
	}
	else
	{
//	    if ( Nx::CEngine::sIsVisible(mid,radius))
#		ifdef __PLAT_NGPS__	
   		if (NxPs2::IsInFrame( mid, radius ))
			DebugRender(ident_matrix, ignore_1, ignore_0, false);
#		endif
	}
#endif	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::DebugRender(const Mth::Matrix & transform, uint32 ignore_1, uint32 ignore_0, bool do_transform)
{
#ifdef	__PLAT_NGPS__	

	uint32	rgb = 0x000000;
	int n = m_num_faces;	
	int r=0;
	int g=0;
	int b=0;
	
	NxPs2::DMAOverflowOK = 2;


	switch (Nx::CEngine::GetWireframeMode())
	{
		case 1:	// Polygon density, red = high, white=higher 
		{
			#define	peak 500	
			if (n <= peak )
			{
				r = (255 * (n)) / peak;  	// r ramps up (black to red)	
			}
			else if (n <= peak * 2)
			{
				r = 255;
				b = (255 * (n - peak) / (peak));	// b&g ramps up (to white)
				g = b;
				
			}
			else
			{
				r = g = b = 255;
			}
			break;
		}
		case 2:	// Low Polygon density, red = high, white=higher 
		{
			#define	peak 500	
			if (n <= 2 )
			{
				r = g = b = 255;			// White = two or less
			}
			else if (n <= 4)
			{
				g = 255;				   // Green = 4 or less
				
			}
			else if (n <= 8)
			{
				b = 255;				   // blue = 8 or less
			}
			else if (n <= 16)			   
			{
				r = b = 255;			   // Magenta = 16 or less
			}
			break;
		}
		case 3:	   	// Each object a different color, based on checksum
		{
			r = (m_checksum >>16) & 0xff;
			g = (m_checksum >>8) & 0xff;
			b = (m_checksum ) & 0xff;
		}
		default:
			break;	
	}


	// fix up anything I did stupid
	if (r<0) r=0;
	if (r>255) r=255;
	if (g<0) g=0;
	if (g>255) g=255;
	if (b<0) b=0;
	if (b>255) b=255;

	rgb = (b<<16)|(g<<8)|r;	
	
	uint32 current = 12345678;


		#if 0
		// scan through all verts, and see if any are close, but not close enough....
		// obviously this will be somewhat slow
		NxPs2::BeginLines3D(0x80000000 + (0x00ff00ff));		// Magenta = unwelded verts

		for (int i=0;i<m_num_verts-1;i++)
		{
			for (int j = i+1; j<m_num_verts;j++)
			{
				float len = (mp_vert_pos[i]-mp_vert_pos[j]).LengthSqr();
				if ( len >0.0000001f && len < 1.0f)
				{
				
					Mth::Vector *v0,*v1;
					v0 = &mp_vert_pos[i];
					v1 = &mp_vert_pos[j];
					NxPs2::DrawLine3D((*v0)[X],(*v0)[Y],(*v0)[Z],(*v0)[X],(*v0)[Y]+100.0f,(*v0)[Z]);								
					NxPs2::DrawLine3D((*v1)[X],(*v1)[Y],(*v1)[Z],(*v1)[X],(*v1)[Y]+100.0f,(*v1)[Z]);								
				}
			}
		}
		NxPs2::EndLines3D();
		return;

		
		#endif


	Mth::Vector v0, v1, v2;
	

	for (int fidx = 0; fidx < m_num_faces; fidx++)
	{
		SFaceInfo *face = get_face_info(fidx);

		if (!(face->m_flags & ignore_1) && !(~face->m_flags & ignore_0))
		{
		
		if (Nx::CEngine::GetWireframeMode() == 0)   // face flags
		{
			rgb = 0xffffff;						// white = no flags
			if (face->m_flags & mFD_VERT)
			{
				rgb = 0x4040ff;			   		// red = vert
			}				
			if (face->m_flags & mFD_TRIGGER)
			{
				rgb = 0xff4040;				    // blue = trigger
			}				
			if (face->m_flags & mFD_WALL_RIDABLE)
			{
				rgb = 0x00ffff;				   // yellow = wallride
			}				
		}			   
				   
			// check for context changes
			if (current != rgb)
			{
				if (current == 12345678)
				{
					NxPs2::BeginLines3D(0x80000000 + (0x00ffffff & rgb));
				}
				else
				{
		    		NxPs2::ChangeLineColor(0x80000000 + (0x00ffffff & rgb));
				}
				current = rgb;
			}

#ifdef FIXED_POINT_VERTICES
			if (do_transform)
			{
				v0 = transform.TransformAsPos(GetRawVertexPos(GetFaceVertIndex(fidx, 0)));
				v1 = transform.TransformAsPos(GetRawVertexPos(GetFaceVertIndex(fidx, 1)));
				v2 = transform.TransformAsPos(GetRawVertexPos(GetFaceVertIndex(fidx, 2)));
			}
			else
			{
				v0 = GetRawVertexPos(GetFaceVertIndex(fidx, 0));
				v1 = GetRawVertexPos(GetFaceVertIndex(fidx, 1));
				v2 = GetRawVertexPos(GetFaceVertIndex(fidx, 2));
			}
#else
			if (do_transform)
			{
				v0 = transform.TransformAsPos(mp_vert_pos[GetFaceVertIndex(fidx, 0)]);
				v1 = transform.TransformAsPos(mp_vert_pos[GetFaceVertIndex(fidx, 1)]);
				v2 = transform.TransformAsPos(mp_vert_pos[GetFaceVertIndex(fidx, 2)]);
			}
			else
			{
				v0 = mp_vert_pos[GetFaceVertIndex(fidx, 0)];
				v1 = mp_vert_pos[GetFaceVertIndex(fidx, 1)];
				v2 = mp_vert_pos[GetFaceVertIndex(fidx, 2)];
			}
#endif // FIXED_POINT_VERTICES

			NxPs2::DrawLine3D(v0[X], v0[Y], v0[Z], v1[X], v1[Y], v1[Z]);
			NxPs2::DrawLine3D(v0[X], v0[Y], v0[Z], v2[X], v2[Y], v2[Z]);
			NxPs2::DrawLine3D(v2[X], v2[Y], v2[Z], v1[X], v1[Y], v1[Z]);
		}
	}
	// only if we actually drew some
	if ( current != 12345678)
	{
		NxPs2::EndLines3D();
	}
#else
	const uint32 rgba = 0x0000FF80;

	for (int fidx = 0; fidx < m_num_faces; fidx++)
	{
		Mth::Vector v0, v1, v2;

		if (do_transform)
		{
#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
			v0 = transform.TransformAsPos(GetRawVertexPos(GetFaceVertIndex(fidx, 0)));
			v1 = transform.TransformAsPos(GetRawVertexPos(GetFaceVertIndex(fidx, 1)));
			v2 = transform.TransformAsPos(GetRawVertexPos(GetFaceVertIndex(fidx, 2)));
#else
			v0 = transform.TransformAsPos(mp_vert_pos[GetFaceVertIndex(fidx, 0)]);
			v1 = transform.TransformAsPos(mp_vert_pos[GetFaceVertIndex(fidx, 1)]);
			v2 = transform.TransformAsPos(mp_vert_pos[GetFaceVertIndex(fidx, 2)]);
#endif // FIXED_POINT_VERTICES
		}
		else
		{
#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
			v0 = GetRawVertexPos(GetFaceVertIndex(fidx, 0));
			v1 = GetRawVertexPos(GetFaceVertIndex(fidx, 1));
			v2 = GetRawVertexPos(GetFaceVertIndex(fidx, 2));
#else
			v0 = mp_vert_pos[GetFaceVertIndex(fidx, 0)];
			v1 = mp_vert_pos[GetFaceVertIndex(fidx, 1)];
			v2 = mp_vert_pos[GetFaceVertIndex(fidx, 2)];
#endif // FIXED_POINT_VERTICES
		}		
		// Draw Triangle
		Gfx::AddDebugLine(v0, v1, rgba, rgba, 1);
		Gfx::AddDebugLine(v1, v2, rgba, rgba, 1);
		Gfx::AddDebugLine(v2, v0, rgba, rgba, 1);
	}
#endif //	__PLAT_NGPS__	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static Mth::Vector x;
static Mth::Vector y;

inline void rot(Mth::Vector &v)
{
	float vx = v[X];
	float vy = v[Z];
	v[X] = vx * x[X] + vy * x[Z];
	v[Z] = vx * y[X] + vy * y[Z];
}




static float	debug_2d_scale =  0.02f;
static Mth::Vector s_overhead_cam_pos;

void	_debug_change_2d_scale(float x)
{
	debug_2d_scale *= x;
	if (debug_2d_scale > 0.5f) debug_2d_scale = 0.5f;
	if (debug_2d_scale < 0.002f) debug_2d_scale = 0.002f;
	
}

void	OverheadLine(Mth::Vector v0, Mth::Vector v1)
{
#ifdef	__PLAT_NGPS__	

	Mth::Vector offset;
	offset.Set(320.0f,0.0f,224.0f);

	v0 -= s_overhead_cam_pos;
	v1 -= s_overhead_cam_pos;

	v0 *= debug_2d_scale;
	v1 *= debug_2d_scale;

	rot(v0);
	rot(v1);

	v0 += offset;
	v1 += offset;
	
	if (v0[X] < 0 && v1[X] < 0) return;
	if (v0[Z] < 0 && v1[Z] < 0) return;
	if (v0[X] > 639 && v1[X] >639) return;
	if (v0[Z] > 447 && v1[Z] > 447) return;


	// Some simple clipping

	NxPs2::DrawLine2D(v0[X], v0[Z], 0.0f, v1[X],  v1[Z],0.0f);

#endif
}


void	CCollObjTriData::DebugRender2D(uint32 ignore_1, uint32 ignore_0, uint32 visible)
{
#ifdef	__PLAT_NGPS__	


	Gfx::Camera *cur_camera = Nx::CViewportManager::sGetCamera( 0 );
	s_overhead_cam_pos = cur_camera->GetPos();

	// get orientation from the camere
	y = cur_camera->GetMatrix()[Z];
	y[Y] = 0.0f;
	y.Normalize();
	x[X] =   y[Z];
	x[Y] = 0.0f;
	x[Z] = - y[X];

	uint32	rgb = visible;		 		// blue = visible
	Mth::Vector v0, v1, v2;
	NxPs2::BeginLines2D(0x80000000 + (0x00ffffff & rgb));
	for (int fidx = 0; fidx < m_num_faces; fidx++)
	{
		SFaceInfo *face = get_face_info(fidx);
		if (!(face->m_flags & ignore_1) && !(~face->m_flags & ignore_0))
		{
			GetRawVertexPos(GetFaceVertIndex(fidx, 0), v0);
			GetRawVertexPos(GetFaceVertIndex(fidx, 1), v1);
			GetRawVertexPos(GetFaceVertIndex(fidx, 2), v2);
			OverheadLine(v0,v1);
			OverheadLine(v1,v2);
			OverheadLine(v2,v0);
		}
	}
	NxPs2::EndLines2D();
#endif
}

void	CCollObjTriData::DebugRender2DBBox(uint32 ignore_1, uint32 ignore_0, uint32 visible)
{
#ifdef	__PLAT_NGPS__	


	Gfx::Camera *cur_camera = Nx::CViewportManager::sGetCamera( 0 );

	// get orientation from the camere
	y = cur_camera->GetMatrix()[Z];
	y[Y] = 0.0f;
	y.Normalize();
	x[X] =   y[Z];
	x[Y] = 0.0f;
	x[Z] = - y[X];

	uint32	rgb = visible;		 		// blue = visible
	
	Mth::Vector v0, v1, v2,v3;
	
	NxPs2::BeginLines2D(0x80000000 + (0x00ffffff & rgb));
	v0 = GetBBox().GetMax();
	v2 = GetBBox().GetMin();
	v1[X] = v2[X];
	v1[Z] = v0[Z];
	v3[X] = v0[X];
	v3[Z] = v2[Z];
	
	OverheadLine(v0,v1);
	OverheadLine(v1,v2);
	OverheadLine(v2,v3);
	OverheadLine(v3,v0);
	NxPs2::EndLines2D();
#endif
}

// draw as an octagon, to simultate the bounding sphere									   
void	CCollObjTriData::DebugRender2DOct(uint32 ignore_1, uint32 ignore_0, uint32 visible)
{
#ifdef	__PLAT_NGPS__	


	Gfx::Camera *cur_camera = Nx::CViewportManager::sGetCamera( 0 );

	// get orientation from the camere
	y = cur_camera->GetMatrix()[Z];
	y[Y] = 0.0f;
	y.Normalize();
	x[X] =   y[Z];
	x[Y] = 0.0f;
	x[Z] = - y[X];

	uint32	rgb = visible;		 		// blue = visible
	
	Mth::Vector v[8];
	
	Mth::Vector offset;
	
	offset.Set(320.0f,0.0f,224.0f);


	// simple culling based on the world bbox
	Mth::Vector mid = (GetBBox().GetMax() + GetBBox().GetMin())/2.0f;
	float	radius = (mid - GetBBox().GetMin()).Length();
	
	float	half = radius * 0.707106f;		


	v[0].Set(-half,0,-half);
	v[2].Set( half,0,-half);
	v[4].Set( half,0, half);
	v[6].Set(-half,0, half);
	v[1].Set( 0     ,0,-radius);
	v[3].Set( radius,0, 0     );
	v[5].Set( 0     ,0, radius);
	v[7].Set(-radius,0, 0     );

	NxPs2::BeginLines2D(0x80000000 + (0x00ffffff & rgb));
	for (int i=0;i<8;i++)
	{
		v[i] += mid;
	}
	for (int i=0;i<8;i++)
	{
		OverheadLine(v[i], v[(i+1)&7]);
	}
	NxPs2::EndLines2D();
#endif
}

#else
// Stub function for
void	CCollObjTriData::DebugRender(uint32 ignore_1, uint32 ignore_0) {}
void	CCollObjTriData::DebugRender(const Mth::Matrix & transform, uint32 ignore_1, uint32 ignore_0, bool do_transform) {}

#endif

// Checks to see if a coll sector has any potential "Uberfrig" holes or seams
// that would be where there is NO ground beneath a point
// Algorithm:
// for each edge or each triangle
//  find the midpoint of the edge
//  for left and right sides
//   check if there is a hole at either of 0.000001, 0.0001, 0.1, 1.0 inches away (perp to edge)
//	 if hole, then check six feet away
//     if collision six feet away, then we classify this as a hole, and flag it in the level 
//		                                (flag the edge, and the point with a hole)
//
// Note: the checks encompass the whole world, not just this sector
#ifdef	__DEBUG_CODE__

bool CheckEdgeAt(Mth::Vector& mid, Mth::Vector& left, float dist, CFeeler& feeler)
{

	float up = 1000.0f;

/*
	// if we are next to a wall, then we need to check from a much greater height
	feeler.SetLine(mid + left * 10.0f + up, mid - left * 10.0f + up);
	if (feeler.GetCollision())
	{
		up = 1000.0f;
	}
	else
	{
		// need to check for wall from both directions, I think
		feeler.FlipDirection();
		if (feeler.GetCollision())
		{
			up = 1000.0f;		
		}
	}
*/
	
	feeler.SetLine(mid + left * dist + Mth::Vector(0.0f,up,0.0f),
				   mid + left * dist + Mth::Vector(0.0f,-1000.0f,0.0f));
//				   mid + left * dist + Mth::Vector(0.0f,-5800.0f,0.0f));
				   
//	feeler.DebugLine(0,255,0);				   
				   
	return feeler.GetCollision(false);	
}									  
									  
void			CheckEdgeForHoles(Mth::Vector v0, Mth::Vector v1)
{
	Mth::Vector	mid = (v0 + v1) / 2.0f;
	Mth::Vector left = v1 - v0;
	left.Normalize();
	if (left[X] == 0.0f && left[Z] == 0.0f)
	{
		// edge is perfectly vertical, so return
		return;
	}

	// make left vector be at right angles to the edge in the XZ plane	
	float x = left[X];
	left[X] = left[Z];
	left[Z] = -x;
	left[Y] = 0;

	CFeeler	feeler;
	
	// first check six feet away, to eliminate around the edge of the level 

	if (!CheckEdgeAt(mid, left, 72.0f,feeler))
	{
		// hole six feet away, so return
//		Gfx::AddDebugLine(mid+left*60.0f, mid+left*60.0f * 10.0f,0xff0000);
		return;
	}


	for (float x = 0.001f; x<0.0015f; x *= 10.0f)   // does 0.001, 0.01, 0.10
	{
		if (!CheckEdgeAt(mid,left,x,feeler))
		{
			// Found a hole!!!
			// we draw bunch of lines of differning length, so we can see it when we zoom in
			
			Gfx::AddDebugLine(v0, v1,0xff00ff);  // magenta = bad edge

//			feeler.DebugLine(0,0,255);			// blue = actual collision line			
			Gfx::AddDebugLine(mid+left*x, mid+left*x + Mth::Vector(0,40,0),0xffff);  
			Gfx::AddDebugLine(mid+left*x, mid+left*x + Mth::Vector(0,200,0),0xffff); 
			Gfx::AddDebugLine(mid+left*x, mid+left*x + Mth::Vector(0,1000,0),0xffff);
		}
	}

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollObjTriData::CheckForHoles()
{

	printf ("\nChecking for unvelded verts in object with %d verts\n",m_num_verts);

	for (int i=0;i<m_num_verts-1;i++)
	{
		for (int j = i+1; j<m_num_verts;j++)
		{
			float len = (GetRawVertexPos(i)-GetRawVertexPos(j)).LengthSqr();
			if ( len >0.0000001f && len < 0.1f)
			{
			
#ifndef __PLAT_NGC__
				Gfx::AddDebugLine(GetRawVertexPos(i), GetRawVertexPos(i) + Mth::Vector(0,400,0),0xff00ff);  
				Gfx::AddDebugLine(GetRawVertexPos(i), GetRawVertexPos(i) + Mth::Vector(0,40,0),0xff00ff);  
#endif		// __PLAT_NGC__
				printf ("Found one, at dist %f,m check the magenta lines\n",len);
			}
		}
	}


	printf ("Then Checking for holes in object with %d faces\n",m_num_faces);
	for (int fidx = 0; fidx < m_num_faces; fidx++)
	{
		
		SFaceInfo *face = get_face_info(fidx);
		if (!(face->m_flags & mFD_NON_COLLIDABLE))
		{
			CheckEdgeForHoles(GetRawVertexPos(GetFaceVertIndex(fidx, 0)),GetRawVertexPos(GetFaceVertIndex(fidx, 1)));
			CheckEdgeForHoles(GetRawVertexPos(GetFaceVertIndex(fidx, 1)),GetRawVertexPos(GetFaceVertIndex(fidx, 2)));
			CheckEdgeForHoles(GetRawVertexPos(GetFaceVertIndex(fidx, 2)),GetRawVertexPos(GetFaceVertIndex(fidx, 0)));
		}
	}

}
#endif

} // namespace Nx

