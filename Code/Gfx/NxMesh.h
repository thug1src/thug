//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       NxMesh.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/15/2002
//****************************************************************************

#ifndef	__GFX_NXMESH_H__
#define	__GFX_NXMESH_H__

                           
#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <core/math.h>
#include <core/math/geometry.h>

namespace Nx
{
	class CTexDict;
	class CCollObjTriData;
	class CHierarchyObject;

class CMesh : public Spt::Class
{

public:
    // The basic interface to the model
    // this is the machine independent part	
    // machine independent range checking, etc can go here	
	CMesh();
    virtual				~CMesh();

	bool				LoadCollision(const char *p_name);			// load mesh collision data
	
	Nx::CHierarchyObject*		GetHierarchy();
	int							GetNumObjectsInHierarchy();
	Nx::CCollObjTriData*		GetCollisionTriDataArray() const { return mp_coll_objects; }
	int							GetCollisionTriDataArraySize() const { return m_num_coll_objects; }
	const Mth::CBBox &			GetCollisionBBox() const { return m_collision_bbox; }

public:
	Nx::CTexDict*		GetTextureDictionary()
	{
		return mp_texDict;
	}
	uint32				GetRemovalMask()
	{
		return m_CASRemovalMask;
	}

protected:
	Nx::CTexDict*		mp_texDict;

	char						m_coll_filename[128];						// collision filename (kept around for unload)
	int							m_num_coll_objects;							// non-cloned collision
	Nx::CCollObjTriData *		mp_coll_objects;
	Mth::CBBox					m_collision_bbox;							// Bounding box of whole mesh

	uint32						m_CASRemovalMask;

	// For mesh heirarchies
	CHierarchyObject*			mp_hierarchyObjects;						// array of hierarchy objects
	int							m_numHierarchyObjects;						// number of hierarchy objects
};

}

#endif // __GFX_NXMESH_H__


