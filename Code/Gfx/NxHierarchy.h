//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       NxHierarchy.h
//* OWNER:          Garrett Jost
//* CREATION DATE:  7/26/2002
//****************************************************************************

#ifndef	__GFX_NXHIERARCHY_H__
#define	__GFX_NXHIERARCHY_H__

                           
#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <core/math.h>

namespace Nx
{

// Hierarchy information available to game.  It is found at the beginning of a geom.<plat> file
// although it is not used by the geometry directly.
class CHierarchyObject
{
public:
						CHierarchyObject();
						~CHierarchyObject();

	void				SetChecksum(uint32 checksum);
	uint32				GetChecksum() const;
	void				SetParentChecksum(uint32 checksum);
	uint32				GetParentChecksum() const;
	void				SetParentIndex(int16 index);
	int16				GetParentIndex() const;
	void				SetBoneIndex(sint8 index);
	sint8				GetBoneIndex() const;

	void				SetSetupMatrix(const Mth::Matrix& mat);
	const Mth::Matrix &	GetSetupMatrix() const;

	//static CHierarchyObject*	sGetHierarchyArray(uint8 *pPipData, int& size);
	
protected:
	uint32				m_checksum;			// Object checksum
	uint32				m_parent_checksum;	// Checksum of parent, or 0 if root object
	int16				m_parent_index;		// Index of parent in the hierarchy array (or -1 if root object)
	sint8				m_bone_index;		// The index of the bone matrix used on this object
	uint8				m_pad_8;
	uint32				m_pad_32;
	Mth::Matrix			m_setup_matrix;		// Initial local to parent matrix
};

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline CHierarchyObject::CHierarchyObject()
{
	m_checksum = 0;
	m_parent_checksum = 0;

	m_parent_index = -1;
	m_bone_index = 0;

	m_setup_matrix.Ident();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline CHierarchyObject::~CHierarchyObject()
{
}

inline void					CHierarchyObject::SetChecksum(uint32 checksum)
{
	m_checksum = checksum;
}

inline uint32				CHierarchyObject::GetChecksum() const
{
	return m_checksum;
}

inline void					CHierarchyObject::SetParentChecksum(uint32 checksum)
{
	m_parent_checksum = checksum;
}

inline uint32				CHierarchyObject::GetParentChecksum() const
{
	return m_parent_checksum;
}

inline void					CHierarchyObject::SetParentIndex(int16 index)
{
	m_parent_index = index;
}

inline int16				CHierarchyObject::GetParentIndex() const
{
	return m_parent_index;
}

inline void					CHierarchyObject::SetBoneIndex(sint8 index)
{
	m_bone_index = index;
}

inline sint8				CHierarchyObject::GetBoneIndex() const
{
	return m_bone_index;
}

inline void					CHierarchyObject::SetSetupMatrix(const Mth::Matrix& mat)
{
	m_setup_matrix = mat;
}

inline const Mth::Matrix &	CHierarchyObject::GetSetupMatrix() const
{
	return m_setup_matrix;
}

}

#endif // __GFX_NXMESH_H__


