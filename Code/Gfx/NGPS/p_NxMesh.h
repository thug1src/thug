//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxMesh.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/15/2002
//****************************************************************************

#ifndef	__GFX_P_NX_MESH_H__
#define	__GFX_P_NX_MESH_H__
    
#include "gfx/nxmesh.h"

#include <core/string/cstring.h>
			
namespace NxPs2
{
	class	CGeomNode;
	struct	sCASData;
}
			 
namespace Nx
{

	class	CHierarchyObject;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/////////////////////////////////////////////////////////////////////////////////////
//
// Here's a machine specific implementation of the CMesh
    
class CPs2Mesh : public CMesh
{
                                      
public:
	CPs2Mesh( uint32* pModelData, int modelDataSize, uint8* pCASData, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume );
	CPs2Mesh( const char* pMeshFileName, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume );
	virtual 			~CPs2Mesh();

	// temporary?
	NxPs2::CGeomNode*	GetGeomNode() { return mp_geomNode; }
	void				HidePolys( uint32 mask );

protected:
	NxPs2::CGeomNode*	mp_geomNode;
	bool				m_polysAlreadyHidden:1;
	bool				m_isPipped:1;
	uint8*				mp_pipData;	// for pip-style geoms that are loaded from a data buffer
	Str::String			m_pipFileName;

	bool				build_casdata_table(uint8* pCASData);
	bool				build_casdata_table(const char* pFileName);
	
	NxPs2::sCASData*	mp_CASData;
	int					m_numCASData;
};


} // Nx

#endif 
