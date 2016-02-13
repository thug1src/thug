///////////////////////////////////////////////////////////////////////////////
// p_NxScene.cpp

#include 	"gfx/Ngc/p_NxScene.h"
#include 	"gfx/Ngc/p_NxSector.h"
#include 	"gfx/Ngc/p_NxGeom.h"

namespace Nx
{


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcScene::CNgcScene( int sector_table_size ) : CScene( sector_table_size )
{
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcScene::DestroySectorMeshes( void )
{
	// Iterate through the list of sectors, removing each one in turn, and requesting the engine to
	// remove the meshes.
	mp_sector_table->IterateStart();
	CSector* pSector = mp_sector_table->IterateNext();
	while( pSector )
	{
		// Access platform dependent data.
//		CNgcSector* pNgcSector = static_cast<CNgcSector *>(pSector);

		// Remove this mesh array from the engine.
//		pNgcSector->DestroyMeshArray();

		pSector = mp_sector_table->IterateNext();
	}
}



/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CScene

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcScene::plat_post_load()
{
	// Now turn the temporary mesh lists into mesh arrays.
	mp_sector_table->IterateStart();
	CSector* pSector = mp_sector_table->IterateNext();
	while( pSector )
	{
		CNgcGeom *p_Ngc_geom = static_cast<CNgcGeom*>(pSector->GetGeom());

		p_Ngc_geom->CreateMeshArray();

		// First time through we just want to count the meshes,
		p_Ngc_geom->RegisterMeshArray( true );

		pSector = mp_sector_table->IterateNext();
	}

	// Now we have counted all the meshes, tell the engine to create the arrays to hold them.
	GetEngineScene()->CreateMeshArrays();
	
	// Now go through and actually add the meshes.
	mp_sector_table->IterateStart();
	pSector = mp_sector_table->IterateNext();
	while( pSector )
	{
		// Access platform dependent data.
		CNgcGeom *p_Ngc_geom = static_cast<CNgcGeom*>(pSector->GetGeom());

		p_Ngc_geom->RegisterMeshArray( false );

		pSector = mp_sector_table->IterateNext();
	}

	// Now all meshes are registered, tell the engine to sort them.
	GetEngineScene()->SortMeshes();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcScene::plat_load_textures(const char* p_name)
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcScene::plat_load_collision(const char* p_name)
{
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcScene::plat_unload_add_scene( void )
{
	// Not sure what this is supposed to do, but added for now to remove annoying stub output.
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Create an empty sector
CSector	*			CNgcScene::plat_create_sector()
{
	CNgcSector *p_Ngc_sector	= new CNgcSector();
	return p_Ngc_sector;
}



} // Namespace Nx  			
				
				

