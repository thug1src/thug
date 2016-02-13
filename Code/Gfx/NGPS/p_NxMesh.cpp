//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxMesh.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/15/2002
//****************************************************************************

#include <gfx/ngps/p_nxmesh.h>

#include <core/string/stringutils.h>
								  
#include <gel/scripting/symboltable.h>

#include <gfx/nx.h>
#include <gfx/ngps/p_nxtexture.h>
#include <gfx/ngps/nx/geomnode.h>
#include <gfx/ngps/nx/mesh.h>
#include <gfx/ngps/nx/scene.h>
#include <gfx/ngps/nx/texture.h>

#include <sys/file/filesys.h>
#include <sys/file/pip.h>

namespace Nx
{

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Mesh::build_casdata_table(uint8* p_casData)
{
	#ifdef __NOPT_ASSERT__
	uint32 version = *((uint32*)p_casData);
	#endif
	p_casData += sizeof(uint32);

	Dbg_MsgAssert( version >= 2, ( "Obsolete version of CAS file.  Please re-export." ) )
		
	m_CASRemovalMask = *((uint32*)p_casData);
	p_casData += sizeof(uint32);

	m_numCASData = *((int*)p_casData);
	p_casData += sizeof(int);

	// temporary lookup table, for the duration of the scene loading
	NxPs2::CreateCASDataLookupTable();

	if ( m_numCASData > 0 )
	{
		// create-a-skater flags
		mp_CASData = new NxPs2::sCASData[m_numCASData];
		
		for ( int i = 0; i < m_numCASData; i++ )
		{
			mp_CASData[i].mask = *((uint32*)p_casData);
			p_casData += sizeof(uint32);

			// this will get converted into a pointer to the DMA data
			int vertIndex = *((int*)p_casData);
			p_casData += sizeof(int);
			
			NxPs2::SetCASDataLookupData( vertIndex, &mp_CASData[i] );
		}		
	}

	return ( m_numCASData > 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Mesh::build_casdata_table(const char* pFileName)
{
	void *pFile = File::Open(pFileName, "rb");
	Dbg_MsgAssert(pFile, ("Couldn't open CAS data file %s\n", pFileName));

	int file_size = File::GetFileSize(pFile);

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	uint8* p_fileBuffer = (uint8*)Mem::Malloc( file_size );
	Mem::Manager::sHandle().PopContext();

	File::Read(p_fileBuffer, file_size, 1, pFile);
	File::Close( pFile );

	bool success = build_casdata_table( p_fileBuffer );

	Mem::Free(p_fileBuffer);
	
	// some debugging information for the artists
	if ( Script::GetInteger( "DebugCAS", Script::NO_ASSERT ) )
	{
		printf( "%s removes %08x: [", pFileName, m_CASRemovalMask );
		for ( int i = 0; i < 32; i++ )
		{
			uint32 mask = ( 1 << i );
			if ( m_CASRemovalMask & mask )
			{
				printf( " %d", i );
			}
		}
		printf( "]\n" );
	}

	return success;
}
	
/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool use_new_geom_format( const char* pMeshFileName )
{
	// "reetest" is a quick kludge to get the skeletal trees
	// up and running...  eventually, pass this in as a parameter
	// from the nx.cpp level...
	if ( strstr( pMeshFileName, "eh_" ) || strstr( pMeshFileName, "reetest" ) )
	{
		// if it's a vehicle, then use the new geomnode format
		return true;
	}
	else
	{
		return false; 
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Mesh::CPs2Mesh(uint32* pModelData, int modelDataSize, uint8* pCASData, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume)
{
	Dbg_Assert( pTexDict );

	mp_texDict = pTexDict;
	
	// polys should only be hidden once
	m_polysAlreadyHidden = false;

	m_isPipped = false;
	m_numCASData = 0;

	// don't do this for models
	if ( isSkin && pCASData )
	{
		build_casdata_table( pCASData );
	}
	
	// GJ:  The geomnodes blow off the texDictOffset, 
	// which means it won't work with skater models, 
	// or the 10 board gameobjs in the boardshop.
	if ( !isSkin && texDictOffset == 0 )
	{		 
		// since we're not actually opening a PIP file
		// we don't need to keep track of it
		m_isPipped = false;

		// this should already be in GEOM format,
		// by the time the data arrives here...
		// (we're not loading up the Pip through the
		// normal Pip::Load process, so we need
		// to remember the pointer to the pip data
		// so that we can delete it later)
		mp_pipData = (uint8*)Mem::Malloc( modelDataSize );
		Dbg_Assert( mp_pipData );
		
		memcpy( mp_pipData, pModelData, modelDataSize );

		// process data in place
		// final arg (0) says to hold it in a database to be rendered later
		// (as opposed to rendering it unconditionally, LOADFLAG_RENDERNOW)
		mp_geomNode = NxPs2::CGeomNode::sProcessInPlace( mp_pipData, 0 );

		// Get hierarchy array
		mp_hierarchyObjects = (CHierarchyObject*) NxPs2::CGeomNode::sGetHierarchyArray(mp_pipData, m_numHierarchyObjects);
	}
	else
	{
		NxPs2::sScene* pTextureDictionary = ((Nx::CPs2TexDict*)mp_texDict)->GetEngineTextureDictionary();
	
		NxPs2::LoadScene( pModelData, modelDataSize, pTextureDictionary, true, true, texDictOffset, doShadowVolume );

		mp_geomNode = NULL;
	}

	// temporary lookup table is no longer needed
	NxPs2::DestroyCASDataLookupTable();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Mesh::CPs2Mesh(const char* pMeshFileName, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume)
{
	Dbg_Assert( pTexDict );

	m_isPipped = false;
	mp_texDict = pTexDict;
	
	m_numCASData = 0;

	// don't do this for models
	if ( isSkin )
	{
		char CASFileName[256];
		strcpy(CASFileName, pMeshFileName);
		Str::LowerCase( CASFileName );
		char* pExt = strstr( CASFileName, "skin.ps2" );
		if ( pExt )
		{
			Dbg_MsgAssert( pExt, ("Couldn't find skin.ps2 extension in %s", CASFileName));
			strcpy(pExt, "cas.ps2");
			build_casdata_table( CASFileName );
		}
	}
	
	// GJ:  The geomnodes blow off the texDictOffset, 
	// which means it won't work with skater models, 
	// or the 10 board gameobjs in the boardshop.
	if ( !isSkin && texDictOffset == 0 )
	{		 
		// change the file name so that it's using the new Geom
		// file format rather than Mdl.
		char msg[512];
		strcpy( msg, pMeshFileName );
		char* pDot = strstr( msg, "." );
		Dbg_Assert( pDot );
		*pDot = NULL;
		sprintf( pDot, ".geom.%s", Nx::CEngine::sGetPlatformExtension() );
		m_pipFileName = msg;
		m_isPipped = true;

//		printf( "about to load pip file %s %d\n", m_pipFileName.getString(), texDictOffset );

		// Pip::Load the new file
		uint8* pPipData = (uint8*)Pip::Load( m_pipFileName.getString() );

		Dbg_Assert( pPipData );

		// process data in place
		// final arg (0) says to hold it in a database to be rendered later
		// (as opposed to rendering it unconditionally, LOADFLAG_RENDERNOW)
		mp_geomNode = NxPs2::CGeomNode::sProcessInPlace( pPipData, 0 );

		// Find CGeomNodeObjects
//		if ( use_new_geom_format(pMeshFileName) )
		{
			// Get hierarchy array
			mp_hierarchyObjects = (CHierarchyObject *) NxPs2::CGeomNode::sGetHierarchyArray(pPipData, m_numHierarchyObjects);
		}
	}
	else
	{
		NxPs2::sScene* pTextureDictionary = ((Nx::CPs2TexDict*)mp_texDict)->GetEngineTextureDictionary();
	
		NxPs2::LoadScene( pMeshFileName, pTextureDictionary, true, true, texDictOffset, doShadowVolume );

		mp_geomNode = NULL;
	}

	// temporary lookup table is no longer needed
	NxPs2::DestroyCASDataLookupTable();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Mesh::~CPs2Mesh()
{
	//if (mp_geomNodeObjects)
	//{
	//	delete[] mp_geomNodeObjects;
	//}

	if ( mp_geomNode )
	{
		// clean up the node we created
		Dbg_Assert( mp_geomNode );
		mp_geomNode->Cleanup();

		// Pip::Unload the file
		if ( m_isPipped )
		{
			Pip::Unload( m_pipFileName.getString() );
		}

		// the file was not loaded using Pip::Load()
		if ( mp_pipData )
		{
			Mem::Free( mp_pipData );
			mp_pipData = NULL;
		}
	}
	else
	{
		NxPs2::sScene* pScene = ((Nx::CPs2TexDict*)mp_texDict)->GetEngineTextureDictionary();
	
		// unload scene
		NxPs2::DeleteScene( pScene );
	}

	if ( mp_CASData )
	{
		delete[] mp_CASData;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Mesh::HidePolys( uint32 mask )
{
	int count = 0;

	// this function should only be called once per mesh
	// because after the ADC bits are squeezed,
	// the pointers are no longer valid...
	// (was causing a crash in the secret level
	// with random peds that were doing poly removal)
	if ( m_polysAlreadyHidden )
	{
		return;
	}

//	printf( "Hiding polys with mask 0x%08x\n", mask);

	for ( int j = 0; j < m_numCASData; j++ )
	{
		Dbg_Assert( mp_CASData );

		if ( mp_CASData[j].mask & mask )
		{
			if ( mp_CASData[j].pADCBit )
			{
				*mp_CASData[j].pADCBit = 0xC000;
			}
			count++;
		}
	}

//	printf( "Hiding %d polys\n", count );

	m_polysAlreadyHidden = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx
				
