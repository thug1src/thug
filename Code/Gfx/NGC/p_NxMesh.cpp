//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxMesh.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/15/2002
//****************************************************************************

#include <core/defines.h>
#include <core/string/stringutils.h>
#include <sys/file/filesys.h>
#include <gfx/nx.h>
#include <gfx/nxtexman.h>
#include <gfx/Ngc/p_nxmesh.h>
#include <gfx/Ngc/p_nxscene.h>
#include <gfx/Ngc/nx/scene.h>
#include <gfx/Ngc/nx/texture.h>

//extern Nx::CNgcScene *p_skater;

namespace Nx
{

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int cmp( const void *p1, const void *p2 )
{
	NxNgc::sCASData *p_casdata1 = (NxNgc::sCASData*)p1;
	NxNgc::sCASData *p_casdata2 = (NxNgc::sCASData*)p2;
	
	uint32 mesh1 = p_casdata1->data0 >> 16;
	uint32 mesh2 = p_casdata2->data0 >> 16;
	
	if( mesh1 > mesh2 )
	{
		return 1;
	}
	else if( mesh1 < mesh2 )
	{
		return -1;
	}
	else
	{
		uint32 indexzero1 = p_casdata1->data0 & 0xFFFF;
		uint32 indexzero2 = p_casdata2->data0 & 0xFFFF;
		if( indexzero1 > indexzero2 )
		{
			return 1;
		}
		else if( indexzero1 < indexzero2 )
		{
			return -1;
		}
	}
	return 0;
}


	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcMesh::build_casdata_table( const char* pFileName )
{
	void *pFile = File::Open( pFileName, "rb" );
	Dbg_MsgAssert( pFile, ( "Couldn't open CAS data file %s\n", pFileName ));

	uint32 version;
	File::Read( &version, sizeof( uint32 ), 1, pFile );

	if( version >= 2 )
	{
		File::Read( &m_CASRemovalMask, sizeof( uint32 ), 1, pFile );
	}
	
	File::Read( &m_numCASData, sizeof( int ), 1, pFile );

	if( m_numCASData > 0 )
	{
		// CAS flags.
		mp_CASData = new NxNgc::sCASData[m_numCASData];
		File::Read( &mp_CASData[0].mask,	sizeof( uint32 ) * 3 * m_numCASData, 1, pFile );
	
		// Sort the CAS data based first on mesh, then on the first tri index, lowest to highest. This allows some efficient early-out checking
		// during the poly removal.
		qsort( mp_CASData, m_numCASData, sizeof( NxNgc::sCASData ), cmp );
	}

	File::Close( pFile );
	return ( m_numCASData > 0 );
}

#define MemoryRead( dst, size, num, src )	memcpy(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcMesh::build_casdata_table_from_memory( void **pp_mem )
{
	uint8 *p_data = (uint8*)( *pp_mem );

	uint32 version;
	MemoryRead( &version, sizeof( uint32 ), 1, p_data );

	if( version >= 2 )
	{
		MemoryRead( &m_CASRemovalMask, sizeof( uint32 ), 1, p_data );
	}
	
	MemoryRead( &m_numCASData, sizeof( int ), 1, p_data );

	if( m_numCASData > 0 )
	{
		// CAS flags.
		mp_CASData = new NxNgc::sCASData[m_numCASData];

		for( uint32 i = 0; i < m_numCASData; ++i )
		{
			MemoryRead( &mp_CASData[i].mask, sizeof( uint32 ) * 3, 1, p_data );
		}

		// Sort the CAS data based first on mesh, then on the first tri index, lowest to highest. This allows some efficient early-out checking
		// during the poly removal.
		qsort( mp_CASData, m_numCASData, sizeof( NxNgc::sCASData ), cmp );
	}
	
	// Set the data pointer to the new position on return.
	*pp_mem = p_data;

	return ( m_numCASData > 0 );
}


/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcMesh::CNgcMesh( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcMesh::CNgcMesh( const char *pMeshFileName )
{
	// Only do the CAS flag building for skinned objects.
	if( strstr( pMeshFileName, "skin" ))
	{
		char CASFileName[256];
		strcpy( CASFileName, pMeshFileName );
		Str::LowerCase( CASFileName );
		char *pExt = strstr( CASFileName, "skin.ngc" );
		if( pExt )
		{
			Dbg_MsgAssert( pExt, ( "Couldn't find skin.ngc extension in %s", CASFileName ));
			strcpy( pExt, "cas.ngc" );
			build_casdata_table( CASFileName );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcMesh::~CNgcMesh( void )
{
	// unload textures
//	NxNgc::DeleteTextures( mp_scene );

	if( mp_CASData )
	{
		delete [] mp_CASData;
	}
	
	if( mp_scene )
	{
		CEngine::sUnloadScene( mp_scene );
//		NxNgc::DeleteScene( mp_engine_scene );
	}
}


void CNgcMesh::SetScene( CNgcScene *p_scene )
{
	mp_scene = p_scene;
	// Copy the hierarchy info from the scene so that above the p-line stuff can access it.
	mp_hierarchyObjects = mp_scene->GetEngineScene()->mp_hierarchyObjects;
	m_numHierarchyObjects = mp_scene->GetEngineScene()->m_numHierarchyObjects;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcMesh::SetCASData( uint8 *p_cas_data )
{
	if( p_cas_data )
	{
		build_casdata_table_from_memory((void**)&p_cas_data );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx
				

