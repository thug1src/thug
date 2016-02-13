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
**	Module:			Nx														**
**																			**
**	File name:		CollTriData.h											**
**																			**
**	Created: 		02/27/2002	-	grj										**
**																			**
*****************************************************************************/

#ifndef	__GEL_COLLTRIDATA_H
#define	__GEL_COLLTRIDATA_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/math.h>
#include <core/math/geometry.h>

#include <gfx/Image/ImageBasic.h>

#include <gel/collision/collenums.h>

#ifdef __PLAT_NGC__
#include <sys/ngc/p_vector.h>
#include <sys/ngc/p_gx.h>
#endif

#include <gfx/nxscene.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// Precision for fixed point split point value
#define COLLISION_SUB_INCH_PRECISION 16.0f
#define COLLISION_RECIPROCAL_SUB_INCH_PRECISION 0.0625f

namespace Nx
{

class CCollStatic;
class CCollMovable;
class CCollStaticTri;
class CCollMovTri;
class CCollObjTriData;
struct CollData;
class CBatchTriCollMan;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/


////////////////////////////////////////////////////////////////
// Axis-Aligned BSP tree node
//
class CCollBSPNode
{
public:
						CCollBSPNode();

	// These functions allow us to have "virtual" functionality with an instance.  We can safely
	// cast a pointer to CCollBSPLeaf if isLeaf() returns true.
	uint8				GetSplitAxis() const { return m_leaf.m_split_axis & 0x3; }
	void				SetSplitAxis(int axis);
	bool				IsNode() const { return (GetSplitAxis() != 3); }
	bool				IsLeaf() const { return (GetSplitAxis() == 3); }

	CCollBSPNode *		GetLessBranch() { Dbg_Assert(IsNode()); return m_node.m_children.GetLessBranch(); }
	CCollBSPNode *		GetGreaterBranch() { Dbg_Assert(IsNode()); return m_node.m_children.GetGreaterBranch(); }

	FaceIndex *			GetFaceIndexArray() { Dbg_Assert(IsLeaf()); return m_leaf.mp_face_idx_array; }

	int					GetSplitPoint() const { return m_node.m_split_point >> NUM_AXIS_BITS; }
	void				SetSplitPoint(int ipoint) { m_node.m_split_point = (ipoint << NUM_AXIS_BITS) | GetSplitAxis(); }
	float				GetFSplitPoint() const { return ((float) GetSplitPoint()) * COLLISION_RECIPROCAL_SUB_INCH_PRECISION; }
	void				SetFSplitPoint(float point) { SetSplitPoint(point * COLLISION_SUB_INCH_PRECISION); }

private:
	////////////////////////////////////////////////////////////////
	// Basically, a class for a pointer that uses the lowest two bits for flags.
	// It also assumes that the pointer points to two consecutive nodes: the left
	// node and the right node.
	//
	class CCollBSPChildren
	{
	public:

		void			Init() { m_left_child_and_flags = 0; }

		// Actual branches
		CCollBSPNode *	GetLeftBranch() const { return GetBasePointer(); }
		CCollBSPNode *	GetRightBranch() const { return GetBasePointer() + 1; }

		// Figures out which branch is which
		CCollBSPNode *	GetLessBranch() const { return IsLeftGreater() ? GetRightBranch() : GetLeftBranch(); }
		CCollBSPNode *	GetGreaterBranch() const { return IsLeftGreater() ? GetLeftBranch() : GetRightBranch(); }

		// These two are needed for the cloning function
		CCollBSPNode *	GetBasePointer() const { return (CCollBSPNode *) (m_left_child_and_flags & ~0x3); }
		void			SetBasePointer(CCollBSPNode *p_base) { m_left_child_and_flags = ((uint32) p_base) | (m_left_child_and_flags & 0x3); }

		bool			IsLeftGreater() const { return m_left_child_and_flags & mLEFT_IS_GREATER; }
		void			SetLeftGreater(bool greater);

	private:

		// Constants
		enum
		{
			mLEFT_IS_GREATER = 0x01,			// Indicates that the left branch is the greater branch
		};

		uint32			m_left_child_and_flags; // points to left branch, right branch one node over
	};		

protected:

   	// Constants
	enum
	{
		NUM_AXIS_BITS = 2,			// Number of bits used in fixed split_point for the axis identification
	};

						~CCollBSPNode();

	// m_split axis must always be in line with the low byte of m_split_point
	struct SNode
	{
		int32				m_split_point;		// the point on the axis (low 2 bits is the axis itself)
		CCollBSPChildren	m_children;			// 32-bit value points to left branch, right branch one node over
	};

	struct SLeaf
	{
#ifdef __PLAT_NGC__		// Big endian on NGC
		uint16				m_num_faces;		// number in faces in face array
		uint8				m_pad1;
		uint8				m_split_axis;		// the axis it is split on (0 = X, 1 = Y, 2 = Z, 3 = Leaf)
#else
		uint8				m_split_axis;		// the axis it is split on (0 = X, 1 = Y, 2 = Z, 3 = Leaf)
		uint8				m_pad1;
		uint16				m_num_faces;		// number in faces in face array
#endif __PLAT_NGC__
		FaceIndex *			mp_face_idx_array;	// leaf
	};

	// Clone functions
	CCollBSPNode *		clone(bool instance = false);
	int					count_bsp_nodes();
	int					count_bsp_face_indices();
	FaceIndex *			find_bsp_face_index_array_start();

	// Modify functions
	void				translate(const Mth::Vector & delta_trans);		// delta since we don't store the original pos
	void				rotate_y(const Mth::Vector & world_origin, Mth::ERot90 rot_y, bool root_node = true);
	void				scale(const Mth::Vector & world_origin, const Mth::Vector & scale, bool root_node = true);

	// The split axis data MUST be in the same place in both SNode and SLeaf
	union
	{
		SNode			m_node;
		SLeaf			m_leaf;
	};

	// Friends
	friend CCollObjTriData;
};

#ifndef __PLAT_NGC__
#define FIXED_POINT_VERTICES
#endif		// __PLAT_NGC__

////////////////////////////////////////////////////////////////
// Collision data for an object
//
class CCollObjTriData
{
public:
	////////////////////////////////////////////////////////////
	// The following structure represent the disk format of
	// the collision data with the correct padding.  If the
	// disk format changes, these structures MUST change, too.
	//
   	struct SReadHeader
	{
		int 	m_version;
		int 	m_num_objects;
		int 	m_total_num_verts;
#ifdef __PLAT_NGC__
		int 	m_total_num_faces;
#else
		int 	m_total_num_faces_large;
		int 	m_total_num_faces_small;
		int		m_total_num_verts_large;
		int		m_total_num_verts_small;
		int		m_pad3;
#endif
	};

	//
						CCollObjTriData();
						~CCollObjTriData();

	bool				InitCollObjTriData(CScene * p_scene, void *p_base_vert_addr, void *p_base_intensity_addr, // Init collision sector after read from file
										   void *p_base_face_addr, void *p_base_node_addr, void *p_base_face_idx_addr);
	bool				InitBSPTree();					// Generate the BSP Tree
	bool				DeleteBSPTree();				// Delete the BSP Tree

	uint32				GetChecksum() const;
	void				SetChecksum(uint32 checksum);	// For cloning
	uint16				GetSectorFlags() const;
	void				SetSectorFlags(uint16 flags);
	void				ClearSectorFlags(uint16 flags);
	const Mth::CBBox &	GetBBox() const;
	int					GetNumVerts() const;
	int					GetNumFaces() const;

	// Vertex functions
#ifdef __PLAT_NGC__
	const unsigned char GetVertexIntensity(int face_idx, int vert_idx) const;
	void				SetVertexIntensity(int face_idx, int vert_idx, unsigned char intensity);

	void				SetVertexPointer( NsVector * p_vertex ) { mp_raw_vert_pos = p_vertex; }
	NsVector *			GetVertexPointer() { return mp_raw_vert_pos; }
#else
	const unsigned char GetVertexIntensity(int vert_idx) const;
	void				SetVertexIntensity(int vert_idx, unsigned char intensity);
#endif		//__PLAT_NGC__ 

	// Face functions
	uint16				GetFaceTerrainType(int face_idx) const;
	uint32				GetFaceFlags(int face_idx) const;
	uint16				GetFaceVertIndex(int face_idx, int vert_num) const;
	const Mth::Vector 	GetFaceNormal(int face_idx) const;
	Mth::Vector			GetRawVertexPos(int vert_idx) const;		// Must copy data for fixed point
	void				GetRawVertexPos(int vert_idx, Mth::Vector & pos) const;
	void				SetRawVertexPos(int vert_idx, const Mth::Vector & pos);
	void				GetRawVertices(Mth::Vector *p_vert_array) const;
	void				SetRawVertices(const Mth::Vector *p_vert_array);

	// Collision functions
	FaceIndex *			FindIntersectingFaces(const Mth::CBBox & line_bbox, uint & num_faces);

	// Clone and move functions
	CCollObjTriData *	Clone(bool instance = false, bool skip_no_verts = false);
	void				Translate(const Mth::Vector & delta_trans);		// delta since we don't store the original pos
	void				RotateY(const Mth::Vector & world_origin, Mth::ERot90 rot_y);
	void				Scale(const Mth::Vector & world_origin, const Mth::Vector & scale);

	void				ProcessOcclusion();

	// Debug functions
	void				DebugRender(uint32 ignore_1, uint32 ignore_0);
	void				DebugRender2D(uint32 ignore_1, uint32 ignore_0, uint32 visible);
	void				DebugRender2DBBox(uint32 ignore_1, uint32 ignore_0, uint32 visible);
	void				DebugRender2DOct(uint32 ignore_1, uint32 ignore_0, uint32 visible);
	void				DebugRender(const Mth::Matrix & transform, uint32 ignore_1, uint32 ignore_0, bool do_transfrom = true);
	void				CheckForHoles();

	// Element size functions for init
	static uint			GetVertElemSize();
	static uint			GetVertSmallElemSize();
	static uint			GetFaceElemSize();
	static uint			GetFaceSmallElemSize();

protected:
	//////////////////////////////////////////////////
	// All of the following member variables and structures
	// match the output of SceneConv.exe EXACTLY.
	// Do not change these without changing SceneConv.

	//////////////////////////////////////////////////
	// Internal structures for the faces (note its size isn't a qword multiple)
	struct SFaceInfo
	{
		uint16			m_flags;				// collision attributes
		uint16			m_terrain_type;			// terrain type
	};

	// Everything in SFace and SFaceSmall before the vert indexes MUST be the same.
	struct SFace
	{
		SFaceInfo		m_info;					// face info
		FaceIndex		m_vertex_index[3];		// indexes into the corresponding vertex array
	};

	struct SFaceSmall
	{
		SFaceInfo		m_info;					// face info
		FaceByteIndex	m_vertex_index[3];		// indexes into the corresponding vertex array
#ifndef __PLAT_NGC__
		uint8			m_pad;
#endif		// __PLAT_NGC__
	};

	// Use this structure to shove the rgba value into the W value of a Vector
	struct SOverlay
	{
		uint32			m_dont_touch_X;			// equivalent to Mth::Vector X
		uint32			m_dont_touch_Y;			// equivalent to Mth::Vector Y
		uint32			m_dont_touch_Z;			// equivalent to Mth::Vector Z
		uint8			m_type;					// equivalent to Mth::Vector W
		uint8			m_intensity;
		uint8			m_pad[2];
	};

	// This structure is used for 16-bit fixed point vertices
	struct SFloatVert
	{
		float			m_pos[3];
//		uint8			m_type;
//		uint8			m_intensity;
//		uint8			m_pad[2];
	};

	// This structure is used for 16-bit fixed point vertices
	struct SFixedVert
	{
		uint16			m_pos[3];			// Relative to min point of bounding box
//		uint8			m_pad;
//		uint8			m_intensity;
	};

	//////////////////////////////////////////////////
	// Members
	uint32				m_checksum;			// checksum of sector
	uint16				m_Flags;			// Sector-level flags
	uint16				m_num_verts;
	uint16				m_num_faces;
	uint8				m_use_face_small;	// Set to 1 if using SFaceSmall below
	uint8				m_use_fixed_verts;

	union {
		SFace *			mp_faces;			// array of faces
		SFaceSmall *	mp_face_small;		// array of small faces
	};

	Mth::CBBox			m_bbox;				// bounding box of sector

#ifdef __PLAT_NGC__
	NsVector *			mp_raw_vert_pos;
#else
	union {
#ifndef FIXED_POINT_VERTICES
		Mth::Vector *	mp_vert_pos;			// array of 32-bit vertices
		SOverlay *		mp_vert_rgba_overlay;	// overlay array of vertex colors for 32-bit vertices
#endif
		SFloatVert *	mp_float_vert;			// array of 32-bit vertices
		SFixedVert *	mp_fixed_vert;			// array of 16-bit vertices
	};
#endif

	CCollBSPNode *		mp_bsp_tree;		// head of BSP tree
	uint8 *				mp_intensity;		// Intensity list
#ifdef __PLAT_NGC__
	NsVector *			mp_cloned_vert_pos;
#else
	uint32				m_pad1;				// padding
#endif		// __PLAT_NGC__

	SFaceInfo *			get_face_info(int face_idx) const;

private:
	// Constants
	enum
	{
		MAX_FACE_INDICIES = 6000 //2048
	};

	// Gets and puts float data (converting to and from fixed point if necessary)
	void				get_float_array_from_data(Mth::Vector *p_float_array) const;
	void				put_float_array_into_data(const Mth::Vector *p_float_array);

	void				set_vertex_pos(int vert_idx, const Mth::Vector & pos);

	CCollBSPNode *		create_bsp_tree(const Mth::CBBox & bbox, FaceIndex *p_face_indexes, uint num_faces, uint level = 1);
	CCollBSPNode *		create_bsp_leaf(FaceIndex *p_face_indexes, uint num_faces);
	bool				calc_split_faces(uint axis, float axis_distance, FaceIndex *p_face_indexes,
										 uint num_faces, uint & less_faces, uint & greater_faces, 
										 FaceIndex *p_less_face_indexes = NULL, FaceIndex *p_greater_face_indexes = NULL);

	void				find_faces(CCollBSPNode *p_bsp_node, const Mth::CBBox & bbox);		// recursively search for faces

	static bool			s_init_tree(CCollBSPNode *p_tree, void *p_base_node_addr, void *p_base_face_idx_addr);

	static const uint	s_max_face_per_leaf;// maximum number faces per leaf
	static const uint	s_max_tree_levels;	// maximum number of levels in a tree

	static FaceIndex	s_face_index_buffer[MAX_FACE_INDICIES];
	static FaceIndex	s_seq_face_index_buffer[MAX_FACE_INDICIES];
	static uint			s_num_face_indicies;

	// Should be readable by all the CCollObj classes
	friend CCollStatic;
	friend CCollMovable;
	friend CCollStaticTri;
	friend CCollMovTri;
	friend CBatchTriCollMan;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CCollObjTriData::SetChecksum(uint32 checksum)
{
	m_checksum = checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint32				CCollObjTriData::GetChecksum() const
{
	return m_checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline int					CCollObjTriData::GetNumVerts() const
{
	return m_num_verts;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline int					CCollObjTriData::GetNumFaces() const
{
	return m_num_faces;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::CBBox &	CCollObjTriData::GetBBox() const
{
	return m_bbox;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint					CCollObjTriData::GetVertElemSize()
{
#ifdef __PLAT_NGC__
	return 1 * 3;
#else
#ifdef FIXED_POINT_VERTICES
	return sizeof(SFloatVert);
#else
	return sizeof(Mth::Vector);
#endif // FIXED_POINT_VERTICES
#endif		// __PLAT_NGC__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint					CCollObjTriData::GetVertSmallElemSize()
{
	return sizeof(SFixedVert);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint					CCollObjTriData::GetFaceElemSize()
{
	return sizeof(SFace);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint					CCollObjTriData::GetFaceSmallElemSize()
{
	return sizeof(SFaceSmall);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint16				CCollObjTriData::GetFaceTerrainType(int face_idx) const
{
	return (m_use_face_small) ? mp_face_small[face_idx].m_info.m_terrain_type : mp_faces[face_idx].m_info.m_terrain_type;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint32				CCollObjTriData::GetFaceFlags(int face_idx) const
{
	return (m_use_face_small) ? mp_face_small[face_idx].m_info.m_flags : mp_faces[face_idx].m_info.m_flags;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint16				CCollObjTriData::GetFaceVertIndex(int face_idx, int vert_num) const
{
	return (m_use_face_small) ? mp_face_small[face_idx].m_vertex_index[vert_num] : mp_faces[face_idx].m_vertex_index[vert_num];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Mth::Vector		 	CCollObjTriData::GetRawVertexPos(int vert_idx) const
{
#if !defined(FIXED_POINT_VERTICES)

#ifdef __PLAT_NGC__
	Mth::Vector v;

	if ( mp_cloned_vert_pos )
	{
		v[X] = mp_cloned_vert_pos[vert_idx].x;
		v[Y] = mp_cloned_vert_pos[vert_idx].y;
		v[Z] = mp_cloned_vert_pos[vert_idx].z;
		v[W] = 0.0f;
	}
	else
	{
		v[X] = mp_raw_vert_pos[vert_idx].x;
		v[Y] = mp_raw_vert_pos[vert_idx].y;
		v[Z] = mp_raw_vert_pos[vert_idx].z;
		v[W] = 0.0f;
	}

	return v;
#else
	return mp_vert_pos[vert_idx];
#endif		// __PLAT_NGC__

#else

	if (m_use_fixed_verts)
	{
		Mth::Vector pos(m_bbox.GetMin());

		pos[X] += ((float) mp_fixed_vert[vert_idx].m_pos[X]) * COLLISION_RECIPROCAL_SUB_INCH_PRECISION;
		pos[Y] += ((float) mp_fixed_vert[vert_idx].m_pos[Y]) * COLLISION_RECIPROCAL_SUB_INCH_PRECISION;
		pos[Z] += ((float) mp_fixed_vert[vert_idx].m_pos[Z]) * COLLISION_RECIPROCAL_SUB_INCH_PRECISION;
		pos[W]	= 0.0f;

		return pos;
	}
	else
	{
		Mth::Vector pos(Mth::Vector::NO_INIT);
		pos[X]			= mp_float_vert[vert_idx].m_pos[X];
		pos[Y]			= mp_float_vert[vert_idx].m_pos[Y];
		pos[Z]			= mp_float_vert[vert_idx].m_pos[Z];
		pos[W]			= 0.0f;
		return pos;
	}
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CCollObjTriData::GetRawVertexPos(int vert_idx, Mth::Vector & pos) const
{
#ifdef __PLAT_NGC__

	if ( mp_cloned_vert_pos )
	{
		pos[X] = mp_cloned_vert_pos[vert_idx].x;
		pos[Y] = mp_cloned_vert_pos[vert_idx].y;
		pos[Z] = mp_cloned_vert_pos[vert_idx].z;
		pos[W] = 0.0f;
	}
	else
	{
		pos[X] = mp_raw_vert_pos[vert_idx].x;
		pos[Y] = mp_raw_vert_pos[vert_idx].y;
		pos[Z] = mp_raw_vert_pos[vert_idx].z;
		pos[W] = 0.0f;
	}
#else
	if (m_use_fixed_verts)
	{
		pos = m_bbox.GetMin();
		pos[X] += ((float) mp_fixed_vert[vert_idx].m_pos[X]) * COLLISION_RECIPROCAL_SUB_INCH_PRECISION;
		pos[Y] += ((float) mp_fixed_vert[vert_idx].m_pos[Y]) * COLLISION_RECIPROCAL_SUB_INCH_PRECISION;
		pos[Z] += ((float) mp_fixed_vert[vert_idx].m_pos[Z]) * COLLISION_RECIPROCAL_SUB_INCH_PRECISION;
		pos[W]	= 0.0f;
	}
	else
	{
#ifdef FIXED_POINT_VERTICES
		pos[X]	= mp_float_vert[vert_idx].m_pos[X];
		pos[Y]	= mp_float_vert[vert_idx].m_pos[Y];
		pos[Z]	= mp_float_vert[vert_idx].m_pos[Z];
#else
		pos		= mp_vert_pos[vert_idx];
#endif
		pos[W]	= 0.0f;
	}
#endif		// __PLAT_NGC__
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __PLAT_NGC__
inline const unsigned char CCollObjTriData::GetVertexIntensity(int face_idx, int vert_idx) const
{
	return mp_intensity[(face_idx * 3) + vert_idx];
}
#else
inline const unsigned char CCollObjTriData::GetVertexIntensity(int vert_idx) const
{
#ifdef FIXED_POINT_VERTICES
	return mp_intensity[vert_idx];
	//if (m_use_fixed_verts)
	//{
	//	return mp_fixed_vert[vert_idx].m_intensity;
	//}
	//else
	//{
	//	return mp_float_vert[vert_idx].m_intensity;
	//}
#else
	{
		return mp_vert_rgba_overlay[vert_idx].m_intensity;
	}
#endif // FIXED_POINT_VERTICES
}
#endif		// __PLAT_NGC__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __PLAT_NGC__
inline void					CCollObjTriData::SetVertexIntensity(int face_idx, int vert_idx, unsigned char intensity)
{
	mp_intensity[(face_idx * 3) + vert_idx] = intensity; 
}
#else
inline void					CCollObjTriData::SetVertexIntensity(int vert_idx, unsigned char intensity)
{
#ifdef FIXED_POINT_VERTICES
	mp_intensity[vert_idx] = intensity;
	//if (m_use_fixed_verts)
	//{
	//	mp_fixed_vert[vert_idx].m_intensity = intensity;
	//}
	//else
	//{
	//	mp_float_vert[vert_idx].m_intensity = intensity;
	//}
#else
	{
		mp_vert_rgba_overlay[vert_idx].m_intensity = intensity;
	}
#endif // FIXED_POINT_VERTICES
}
#endif		// __PLAT_NGC__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline CCollObjTriData::SFaceInfo *	CCollObjTriData::get_face_info(int face_idx) const
{
	return (m_use_face_small) ? &(mp_face_small[face_idx].m_info) : &(mp_faces[face_idx].m_info);
}

} // namespace Nx

#endif  //	__GEL_COLLTRIDATA_H
