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
#include <gfx/xbox/p_nxmesh.h>
#include <gfx/xbox/p_nxscene.h>
#include <gfx/xbox/nx/scene.h>
#include <gfx/xbox/nx/texture.h>

//extern Nx::CXboxScene *p_skater;

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
	NxXbox::sCASData *p_casdata1 = (NxXbox::sCASData*)p1;
	NxXbox::sCASData *p_casdata2 = (NxXbox::sCASData*)p2;
	
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
bool CXboxMesh::build_casdata_table( const char* pFileName )
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
		mp_CASData = new NxXbox::sCASData[m_numCASData];

		for( uint32 i = 0; i < m_numCASData; ++i )
		{
			File::Read( &mp_CASData[i].mask, sizeof( uint32 ) * 3, 1, pFile );
		}

		// Sort the CAS data based first on mesh, then on the first tri index, lowest to highest. This allows some efficient early-out checking
		// during the poly removal.
		qsort( mp_CASData, m_numCASData, sizeof( NxXbox::sCASData ), cmp );
	}

	File::Close( pFile );
	
	return ( m_numCASData > 0 );
}



#define MemoryRead( dst, size, num, src )	CopyMemory(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxMesh::build_casdata_table_from_memory( void **pp_mem )
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
		mp_CASData = new NxXbox::sCASData[m_numCASData];

		for( uint32 i = 0; i < m_numCASData; ++i )
		{
			MemoryRead( &mp_CASData[i].mask, sizeof( uint32 ) * 3, 1, p_data );
		}

		// Sort the CAS data based first on mesh, then on the first tri index, lowest to highest. This allows some efficient early-out checking
		// during the poly removal.
		qsort( mp_CASData, m_numCASData, sizeof( NxXbox::sCASData ), cmp );
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
CXboxMesh::CXboxMesh( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxMesh::CXboxMesh( const char *pMeshFileName )
{
	// Only do the CAS flag building for skinned objects.
	if( strstr( pMeshFileName, "skin" ))
	{
		char CASFileName[256];
		strcpy( CASFileName, pMeshFileName );
		Str::LowerCase( CASFileName );
		char *pExt = strstr( CASFileName, "skin.xbx" );
		if( pExt )
		{
			Dbg_MsgAssert( pExt, ( "Couldn't find skin.xbx extension in %s", CASFileName ));
			strcpy( pExt, "cas.xbx" );
			build_casdata_table( CASFileName );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxMesh::~CXboxMesh( void )
{
	if( mp_CASData )
	{
		delete [] mp_CASData;
	}
	if( mp_scene )
	{
		CEngine::sUnloadScene( mp_scene );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxMesh::SetCASData( uint8 *p_cas_data )
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
void CXboxMesh::SetScene( CXboxScene *p_scene )
{
	mp_scene = p_scene;

	// Copy the hierarchy info from the scene so that above the p-line stuff can access it.
	mp_hierarchyObjects		= mp_scene->GetEngineScene()->mp_hierarchyObjects;
	m_numHierarchyObjects	= mp_scene->GetEngineScene()->m_numHierarchyObjects;

	// Now that we have a scene attached, resolve any cas flag data into specific indexes in the mesh data.
	if( mp_CASData )
	{
		NxXbox::sCASData *p_cas_entry = mp_CASData;
		for( uint32 entry = 0; entry < m_numCASData; ++entry, ++p_cas_entry )
		{
			// Get the mesh this entry references.
			uint32			load_order	= p_cas_entry->data0 >> 16;
			NxXbox::sMesh	*p_mesh		= p_scene->GetEngineScene()->GetMeshByLoadOrder( load_order );
			if( p_mesh )
			{
				// Get the indices of the poly referenced.
				uint32 i0 = p_cas_entry->data0 & 0xFFFF;
				uint32 i1 = p_cas_entry->data1 >> 16;
				uint32 i2 = p_cas_entry->data1 & 0xFFFF;

				// For every vertex index in this mesh...
				uint16 *p_indices	= p_mesh->mp_index_buffer[0];
				uint32 index0		= p_indices[0] & 0x7FFF;
				uint32 index1		= p_indices[1] & 0x7FFF;
				for( uint32 i = 2; i < p_mesh->m_num_indices[0]; ++i )
				{
					uint32 index2 = p_indices[i] & 0x7FFF;
					if(( index0 == i0 ) && ( index1 == i1 ) && ( index2 == i2 ))
					{
						p_cas_entry->start_index = i - 2;
						break;
					}
					index0	= index1;
					index1	= index2;
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx
				
