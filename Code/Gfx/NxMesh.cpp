//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       NxMesh.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/15/2002
//****************************************************************************

#include <gfx/nx.h>
#include <gfx/nxmesh.h>
#include <gfx/nxhierarchy.h>

#include <gel/collision/collision.h>
#include <gel/collision/colltridata.h>

#include <sys/file/pip.h>

#ifdef __PLAT_NGC__
#include <gfx/ngc/p_nxmesh.h>
#endif		// __PLAT_NGC__

namespace Nx
{

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions are provided here:
// so engine implementors can leave certain functionality until later
						
/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

// These functions are the platform independent part of the interface to 
// the platform specific code
// parameter checking can go here....
// although we might just want to have these functions inline, or not have them at all?


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMesh::CMesh()
{
	m_CASRemovalMask = 0;

	// In case it isn't loaded below the p-line
	mp_hierarchyObjects = NULL;
	m_numHierarchyObjects = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMesh::~CMesh()
{
	// Remove Collision
	if (mp_coll_objects)
	{
		Pip::Unload(m_coll_filename);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CMesh::LoadCollision(const char *p_name)
{
	
	// for now collision is kind of assumed to be platform independent 
	strcpy(m_coll_filename, p_name);
	char *p_ext = strstr(m_coll_filename, ".");
	if (p_ext)
	{
		strcpy(p_ext, ".col.");
	} else {
		strcat(m_coll_filename, ".col.");
	}
	strcat(m_coll_filename, CEngine::sGetPlatformExtension());

//	Dbg_Message ( "Loading collision %s....", m_coll_filename );

	Mem::PushMemProfile((char*)m_coll_filename);

	uint8 *p_base_addr = (uint8 *) Pip::Load(m_coll_filename);
	if (p_base_addr)
	{
		Nx::CCollObjTriData::SReadHeader *p_header = (Nx::CCollObjTriData::SReadHeader *) p_base_addr;
		p_base_addr += sizeof(Nx::CCollObjTriData::SReadHeader);

//		Dbg_Message ( "Version # %d header sizeof %d", p_header->m_version, sizeof(Nx::CCollObjTriData));
#ifdef __PLAT_NGC__
		Dbg_Message ( "Number of objects: %d verts: %d faces: %d", p_header->m_num_objects, p_header->m_total_num_verts, p_header->m_total_num_faces );
#else
//		Dbg_Message ( "Number of objects: %d verts: %d faces: %d", p_header->m_num_objects, p_header->m_total_num_verts, p_header->m_total_num_faces_large + p_header->m_total_num_faces_small);
//		Dbg_Message ( "Small (%d) verts: %d Large (%d) verts: %d", Nx::CCollObjTriData::GetVertSmallElemSize(), p_header->m_total_num_verts_small, Nx::CCollObjTriData::GetVertElemSize(), p_header->m_total_num_verts_large);
#endif		// __PLAT_NGC__
		Dbg_MsgAssert(p_header->m_version >= 9, ("Collision version must be at least 9."));

		// reserve space for objects
		m_num_coll_objects = p_header->m_num_objects;
		mp_coll_objects = (Nx::CCollObjTriData *) p_base_addr;

		// Calculate base addresses for vert and face arrays
		uint8 *p_base_vert_addr = (uint8 *) (mp_coll_objects + m_num_coll_objects);
#ifndef __PLAT_NGC__
		p_base_vert_addr = (uint8 *)(((uint)(p_base_vert_addr+15)) & 0xFFFFFFF0);	// Align to 128 bit boundary
#ifdef FIXED_POINT_VERTICES
		uint8 *p_base_intensity_addr = p_base_vert_addr + (p_header->m_total_num_verts_large * Nx::CCollObjTriData::GetVertElemSize() +
														   p_header->m_total_num_verts_small * Nx::CCollObjTriData::GetVertSmallElemSize());
		uint8 *p_base_face_addr = p_base_intensity_addr + p_header->m_total_num_verts;
		p_base_face_addr = (uint8 *)(((uint)(p_base_face_addr+3)) & 0xFFFFFFFC);	// Align to 32 bit boundary
#else
		uint8 *p_base_intensity_addr = NULL;
		uint8 *p_base_face_addr = p_base_vert_addr + (p_header->m_total_num_verts * Nx::CCollObjTriData::GetVertElemSize());
		p_base_face_addr = (uint8 *)(((uint)(p_base_face_addr+15)) & 0xFFFFFFF0);	// Align to 128 bit boundary
#endif // FIXED_POINT_VERTICES
#else
		uint8 *p_base_face_addr = p_base_vert_addr + (p_header->m_total_num_faces * Nx::CCollObjTriData::GetVertElemSize());
		p_base_face_addr = (uint8 *)(((uint)(p_base_face_addr+3)) & 0xFFFFFFFC);	// Align to 32 bit boundary
#endif		// __PLAT_NGC__

		// Calculate addresses for BSP arrays
#ifndef __PLAT_NGC__
		uint8 *p_node_array_size = p_base_face_addr + (p_header->m_total_num_faces_large * Nx::CCollObjTriData::GetFaceElemSize() +
													   p_header->m_total_num_faces_small * Nx::CCollObjTriData::GetFaceSmallElemSize());
		p_node_array_size += ( p_header->m_total_num_faces_large & 1 ) ? 2 : 0;
#else
		uint8 *p_node_array_size = p_base_face_addr + ( p_header->m_total_num_faces * Nx::CCollObjTriData::GetFaceElemSize() );
		p_node_array_size += ( p_header->m_total_num_faces & 1 ) ? 2 : 0;
#endif		// __PLAT_NGC__
		uint8 *p_base_node_addr = p_node_array_size + 4;
		uint8 *p_base_face_idx_addr = p_base_node_addr + *((int *) p_node_array_size);

		// Read objects
		for (int oidx = 0; oidx < p_header->m_num_objects; oidx++)
		{
#ifdef __PLAT_NGC__
			CScene * p_scene = static_cast<CScene*>( (static_cast<CNgcMesh*>( this ))->GetScene() );
			mp_coll_objects[oidx].InitCollObjTriData(p_scene, p_base_vert_addr, NULL, p_base_face_addr, p_base_node_addr, p_base_face_idx_addr);
#else
			mp_coll_objects[oidx].InitCollObjTriData(NULL, p_base_vert_addr, p_base_intensity_addr, p_base_face_addr,
													 p_base_node_addr, p_base_face_idx_addr);
#endif		// __PLAT_NGC__
			mp_coll_objects[oidx].InitBSPTree();

			// Add to mesh bbox
			m_collision_bbox.AddPoint(mp_coll_objects[oidx].GetBBox().GetMin());
			m_collision_bbox.AddPoint(mp_coll_objects[oidx].GetBBox().GetMax());
		}

//		Dbg_Message("Mesh bounding box: min (%f, %f, %f) max (%f, %f, %f)", 
//					m_collision_bbox.GetMin()[X], m_collision_bbox.GetMin()[Y], m_collision_bbox.GetMin()[Z], 
//					m_collision_bbox.GetMax()[X], m_collision_bbox.GetMax()[Y], m_collision_bbox.GetMax()[Z]);

	} else {
		Dbg_Error ( "Could not open collision file\n" );
		return false;
	}

//	Dbg_Message ( "successfully loaded collision" );

	if (m_num_coll_objects > 0)
	{
#if 0
		// Add to CSectors
		for (int i = 0; i < m_num_coll_objects; i++)
		{
			CSector *p_sector = GetSector(mp_coll_objects[i].GetChecksum());
			if (p_sector)	// Don't assert now since there may not be renderable data
			{
				Dbg_MsgAssert(p_sector, ("LoadCollision: Can't find CSector with checksum %x", mp_coll_objects[i].GetChecksum()));
				p_sector->AddCollSector(&(mp_coll_objects[i]));
			}
		}
#endif
	}

	Mem::PopMemProfile(/*(char*)m_coll_filename*/);

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::CHierarchyObject* CMesh::GetHierarchy()
{
	return mp_hierarchyObjects;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CMesh::GetNumObjectsInHierarchy()
{
	return m_numHierarchyObjects;
}


} // Nx

