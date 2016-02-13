//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxMesh.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/15/2002
//****************************************************************************

#ifndef	__GFX_P_NX_MESH_H__
#define	__GFX_P_NX_MESH_H__
    
#include "gfx/nxmesh.h"
#include "p_nxscene.h"

namespace NxNgc
{
	struct sScene;
}
			 
namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/////////////////////////////////////////////////////////////////////////////////////
//
// Here's a machine specific implementation of the CMesh
    
class CNgcMesh : public CMesh
{
                                      
public:
						CNgcMesh( void );
						CNgcMesh( const char *pMeshFileName );
	virtual 			~CNgcMesh();
	void				SetScene( CNgcScene *p_scene );

	void				SetTexDict( Nx::CTexDict *p_tex_dict )	{ mp_texDict = p_tex_dict; }
	void				SetCASData( uint8 *p_cas_data );
	CNgcScene			*GetScene( void )						{ return mp_scene; }

	NxNgc::sCASData		*GetCASData( void )						{ return mp_CASData; }
	uint32				GetNumCASData( void )					{ return m_numCASData; }

protected:
	bool				build_casdata_table(const char* pFileName);
	bool				build_casdata_table_from_memory( void **pp_mem );
	
	NxNgc::sCASData		*mp_CASData;
	uint32				m_numCASData;

private:
	CNgcScene			*mp_scene;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // Nx

#endif 

