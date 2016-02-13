///////////////////////////////////////////////////////////////////////////////
// p_NxScene.cpp

#include 	"Gfx/nx.h"
#include 	"Gfx/NGPS/p_NxGeom.h"
#include 	"Gfx/NGPS/p_NxScene.h"
#include 	"Gfx/NGPS/p_NxSector.h"
#include 	"Gfx/NGPS/NX/geomnode.h"

#include <libgraph.h>

#include <sys/file/pip.h>

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Scene::CPs2Scene()
{
	mp_plat_scene = NULL;
	mp_plat_clone_scene = NULL;
	mp_plat_add_scene = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Scene::~CPs2Scene()
{
	if (mp_plat_clone_scene)
	{
		// Have to delete the sectors here first
		mp_sector_table->IterateStart();
		CSector *p_sector;
		while ((p_sector = mp_sector_table->IterateNext()))
		{
			p_sector->clear_flags(CSector::mIN_SUPER_SECTORS);	// Tells cloned sectors it is OK to go away (and SuperSectors will be dead, anyway)
			delete p_sector;
		}
		delete mp_sector_table;
		mp_sector_table = NULL;

		mp_plat_clone_scene->Cleanup();
		delete mp_plat_clone_scene;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

NxPs2::CGeomNode *	CPs2Scene::GetEngineScene() const
{
	return mp_plat_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2Scene::SetEngineScene(NxPs2::CGeomNode *p_scene)
{
	mp_plat_scene = p_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

NxPs2::CGeomNode *	CPs2Scene::GetEngineCloneScene() const
{
	return mp_plat_clone_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2Scene::SetEngineCloneScene(NxPs2::CGeomNode *p_scene)
{
	mp_plat_clone_scene = p_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

NxPs2::CGeomNode *	CPs2Scene::GetEngineAddScene() const
{
	return mp_plat_add_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2Scene::SetEngineAddScene(NxPs2::CGeomNode *p_scene)
{
	mp_plat_add_scene = p_scene;
}

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CScene

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2Scene::create_sectors(NxPs2::CGeomNode *p_node)
{
	if (!p_node)
		return;

	while (p_node)
	{
		// Recursively traverse tree, treating the object-level nodes as leaves.
		// From each object, create a sector.
		// (Mick: optimized tail recursion, after suggestion from John Brandwood)
		if (p_node->IsObject())			// Create CSector from object
		{
			CSector *pSector;
			CPs2Sector *pPs2Sector;		// platform dependent class
			CPs2Geom *pGeom;
	
			// Find Sector
			pSector = GetSector(p_node->GetChecksum());
			Dbg_MsgAssert(!pSector, ("We already have a sector with checksum %x", p_node->GetChecksum()));
	
			// If it doesn't exist yet, create it
			if (!pSector)
			{
				pSector = new CPs2Sector;		// Create platform specific data
				pSector->SetChecksum(p_node->GetChecksum());
	
				pGeom = new CPs2Geom;
				pGeom->SetEngineObject(p_node);
	
				pSector->SetGeom(pGeom);
	
				AddSector(pSector);
	
				//Dbg_Message("Creating new sector %x", p_node->GetChecksum());
			}
	
			// Access platform dependent data
			pPs2Sector = static_cast<CPs2Sector *>(pSector);
	
			// Add node to sector
			//pPs2Sector->SetEngineObject(p_node);
			pPs2Sector->SetEngineObject(NULL);
		} else {	// Traverse children and siblings
			create_sectors(p_node->GetChild());
		}
	
		p_node = p_node->GetSibling();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2Scene::add_sectors(NxPs2::CGeomNode *p_node)
{
	if (!p_node)
		return;

	// Recursively traverse tree, treating the object-level nodes as leaves.
	// From each object, create a sector.
	if (p_node->IsObject())			// Create CSector from object
	{
		CSector *pSector, *p_found_sector;
		CPs2Sector *pPs2Sector;		// platform dependent class
		CPs2Geom *pGeom;

		// Find Sector
		p_found_sector = GetSector(p_node->GetChecksum());
		//Dbg_MsgAssert(!pSector, ("We already have a sector with checksum %x", p_node->GetChecksum()));

		// Even if it does exist, create it
		pSector = new CPs2Sector;		// Create platform specific data
		pSector->SetChecksum(p_node->GetChecksum());

		pGeom = new CPs2Geom;
		pGeom->SetEngineObject(p_node);

		pSector->SetGeom(pGeom);

		#ifdef	__NOPT_ASSERT__
		CSector *p_old_sector =
		#endif
		ReplaceSector(pSector);

		Dbg_Assert(p_old_sector == p_found_sector);
		//Dbg_Message("Creating new sector %x", p_node->GetChecksum());

		// Access platform dependent data
		pPs2Sector = static_cast<CPs2Sector *>(pSector);

		// Add node to sector
		pPs2Sector->SetEngineObject(NULL);
	} else {	// Traverse children and siblings
		add_sectors(p_node->GetChild());
	}

	add_sectors(p_node->GetSibling());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2Scene::delete_sectors(NxPs2::CGeomNode *p_node)
{
	if (!p_node)
		return;

	// Recursively traverse tree, treating the object-level nodes as leaves.
	// From each object, delete the sector.
	if (p_node->IsObject())			// Create CSector from object
	{
		// Find Sector
		CSector *pSector = GetSector(p_node->GetChecksum());
		Dbg_MsgAssert(pSector, ("Cant find a sector with checksum %x to delete", p_node->GetChecksum()));

		DeleteSector(pSector);
	} else {	// Traverse children and siblings
		delete_sectors(p_node->GetChild());
	}

	delete_sectors(p_node->GetSibling());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2Scene::plat_post_load()
{
	// Generate CSectors first
	// Garrett: TEMP HACK: We don't want to traverse siblings, so we send down the child.
	//          This works because we know there won't be any data in the root node.
	//          THIS WILL GO AWAY VERY SOON.  DON'T COPY THIS HACK WITHOUT THIS MESSAGE.
	create_sectors(mp_plat_scene->GetChild());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2Scene::plat_post_add()
{
	// Generate CSectors first
	// Garrett: TEMP HACK: We don't want to traverse siblings, so we send down the child.
	//          This works because we know there won't be any data in the root node.
	//          THIS WILL GO AWAY VERY SOON.  DON'T COPY THIS HACK WITHOUT THIS MESSAGE.
	add_sectors(mp_plat_add_scene->GetChild());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2Scene::plat_load_textures(const char *p_name)
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2Scene::plat_load_collision(const char *p_name)
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2Scene::plat_add_collision(const char *p_name)
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2Scene::plat_unload_add_scene()
{
	if (mp_plat_add_scene)
	{
		sceGsSyncPath(0, 0);

		// Delete CSectors first
		// Garrett: TEMP HACK: We don't want to traverse siblings, so we send down the child.
		//          This works because we know there won't be any data in the root node.
		//          THIS WILL GO AWAY VERY SOON.  DON'T COPY THIS HACK WITHOUT THIS MESSAGE.
		//delete_sectors(mp_plat_add_scene->GetChild());

		// clean up the node we created
		mp_plat_add_scene->Cleanup();

		// Pip::Unload the file
		Pip::Unload(m_add_scene_filename);
		m_add_scene_filename[0] = '\0';

		return true;
	}

	return false;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Create an empty sector
CSector	*			CPs2Scene::plat_create_sector()
{
	CPs2Sector * p_sector = new CPs2Sector(); 

	return p_sector;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void				CPs2Scene::plat_set_majority_color(Image::RGBA rgba)
{
	if (mp_plat_scene)
	{
		// Set the color of the root node
		mp_plat_scene->SetColored(true);
		mp_plat_scene->SetColor(*((uint32 *) &rgba));
	}
	if (mp_plat_clone_scene)
	{
		// Set the color of the root clone node
		mp_plat_clone_scene->SetColored(true);
		mp_plat_clone_scene->SetColor(*((uint32 *) &rgba));
	}
}
		 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Image::RGBA			CPs2Scene::plat_get_majority_color() const
{
	uint32 color = mp_plat_scene->GetColor();
	return *((Image::RGBA *) &color);
}
	

} // Namespace Nx  			
				
				
