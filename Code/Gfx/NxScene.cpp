///////////////////////////////////////////////////////////////////////////////////
// NX.CPP - Platform independent interface to the platfrom specific engine code 

#include "gfx/nx.h"
#include "gfx/NxSector.h"
#include "gfx/NxTexMan.h"

#include "core/debug.h"

#include <gel/collision/collision.h>
#include <gel/collision/colltridata.h>


#include <sys/file/filesys.h>
#include <sys/file/pip.h>

#include <gel/scripting/script.h>	   
#include <gel/scripting/struct.h>	   
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>

#include <gfx\nxgeom.h>

#ifdef	__PLAT_NGPS__
// For wireframe debuging mode
#include <gfx\ngps\nx\line.h>
#include <gfx\ngps\nx\geomnode.h>
#include <gfx\ngps\p_nxgeom.h>
#include <gfx\ngps\p_nxscene.h>
#endif

#include <gfx\nxweather.h>

namespace	Nx
{

///////////////////////////////////////////////////////////////////
// CScene definitions

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CScene::CScene( int sector_table_size ) : mp_sector_man(NULL)
{
	mp_sector_table = new Lst::HashTable< CSector >( sector_table_size );

//	Dbg_MsgAssert(0,("secotr-tabel_size = %d\n",sector_table_size))

	m_scene_filename[0] = '\0';
	
	// Initilizing supersector and collision crap to NULL, as 
	// we might not have any (like for skys and background stuff)
	mp_sector_man = NULL;
	mp_coll_sectors = NULL;
	mp_coll_sector_data = NULL;
	m_num_coll_sectors = 0;

	// Incremental update
	m_add_scene_filename[0] = '\0';
	mp_add_tex_dict = NULL; 
	mp_add_coll_sectors = NULL;
	mp_add_coll_sector_data = NULL;
	m_num_add_coll_sectors = 0;

	m_using_add_sectors = false;
	mp_orig_sectors = new Lst::Head < CSector >;
	mp_add_sectors = new Lst::Head < CSector >;

	m_in_super_sectors = false;
	m_sky = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CScene::~CScene()
{

	// Mick: iterate over all the sectors, and delete them
	// (deleting the mp_sector_table just deletes the references to them)
	if (mp_sector_table)
	{
		mp_sector_table->IterateStart();
		CSector *p_sector;
		while ((p_sector = mp_sector_table->IterateNext()))
		{
			p_sector->clear_flags(CSector::mIN_SUPER_SECTORS);	// Tells cloned sectors it is OK to go away (and SuperSectors will be dead, anyway)
			delete p_sector;
		}
		delete mp_sector_table;
	}
	
	// Remove SuperSectors
	if (mp_sector_man)
	{
		delete mp_sector_man;
	}
	
	// Remove Collision
	if (mp_coll_sectors)
	{
		delete[] mp_coll_sectors;
	}

	if (mp_coll_sector_data)
	{
		Pip::Unload(m_coll_filename);
	}

	// And the toggle list heads
	if (mp_orig_sectors)
	{
		delete mp_orig_sectors;
	}

	if (mp_add_sectors)
	{
		delete mp_add_sectors;
	}

	// Check if additional texture dictionary was loaded
	if (mp_add_tex_dict) 
	{
		Nx::CTexDictManager::sUnloadTextureDictionary(mp_add_tex_dict);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::UnloadAddScene()
{
	if (m_using_add_sectors)
	{
	   	ToggleAddScene();
	}

	if (!plat_unload_add_scene())
	{
		return false;
	}

	if (mp_add_coll_sectors)
	{
		delete[] mp_add_coll_sectors;
	}

	if (mp_add_coll_sector_data)
	{
		Pip::Unload(m_add_coll_filename);
	}

	if (mp_orig_sectors)
	{
		Lst::Node< CSector > *obj_node, *next;

		for(obj_node = mp_orig_sectors->GetNext(); obj_node; obj_node = next)
		{
			next = obj_node->GetNext();

			delete obj_node;
		}
	}

	if	(mp_add_sectors)
	{
		Lst::Node< CSector > *obj_node, *next;

		for(obj_node = mp_add_sectors->GetNext(); obj_node; obj_node = next)
		{
			next = obj_node->GetNext();

			DeleteSector(obj_node->GetData());		// And delete the sector itself
			delete obj_node;
		}
	}

	m_using_add_sectors = false;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSector * 		CScene::GetSector(uint32 sector_checksum)
{
	return mp_sector_table->GetItem(sector_checksum);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 			CScene::AddSector(CSector *pSector)
{
	mp_sector_table->PutItem(pSector->GetChecksum(), pSector);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSector *		CScene::ReplaceSector(CSector *pSector)
{
	CSector *p_old_sector = mp_sector_table->GetItem(pSector->GetChecksum());

	// Possibly delete old sector
	if (p_old_sector)
	{
		//if (p_old_sector->get_flags() & CSector::mCLONE)
		//{
		//	DeleteSector(p_old_sector);
		//} else {
			remove_sector_from_table(p_old_sector->GetChecksum());	// can't delete something that is PIP'ed in
			p_old_sector->SetActive(false);
		//}

		// We also want this out of the SuperSectors
		p_old_sector->set_flags(CSector::mREMOVE_FROM_SUPER_SECTORS);

		// Also add to orig list
		Lst::Node<CSector> *node = new Lst::Node<CSector>(p_old_sector);
		mp_orig_sectors->AddToTail(node);

		Dbg_Message("Replacing sector %x", pSector->GetChecksum());
	} else {
		Dbg_Message("Adding sector %x", pSector->GetChecksum());
	}

	// And add new one
	// Garrett TODO: Should I check to see if there is a CCollObj here?  Or UpdateSuperSectors()?  Or does it matter?
	pSector->set_flags(CSector::mADD_TO_SUPER_SECTORS);			// And do an UpdateSuperSectors() after all the calls
	AddSector(pSector);

	Lst::Node<CSector> *node = new Lst::Node<CSector>(pSector);
	mp_add_sectors->AddToTail(node);

	return p_old_sector;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::ToggleAddScene()
{
	if (mp_add_sectors->CountItems() == 0)
	{
		return false;
	}

	Lst::Head < CSector > *p_on_sectors, *p_off_sectors;
	Lst::Node< CSector > *obj_node, *next;
	CSector *p_sector;

	// Do actual toggle
	m_using_add_sectors = !m_using_add_sectors;

	// Figure out which to turn on and off
	if (m_using_add_sectors)
	{
		p_on_sectors = mp_add_sectors;
		p_off_sectors = mp_orig_sectors;
	} else {
		p_on_sectors = mp_orig_sectors;
		p_off_sectors = mp_add_sectors;
	}

	// Do off list first
	for(obj_node = p_off_sectors->GetNext(); obj_node; obj_node = next)
	{
		next = obj_node->GetNext();
		p_sector = obj_node->GetData();

		p_sector->set_flags(CSector::mREMOVE_FROM_SUPER_SECTORS);
		remove_sector_from_table(p_sector->GetChecksum());
		p_sector->SetActive(false);
		//Dbg_Message("Removed sector %x in toggle", p_sector->GetChecksum());
	}

	// Do on list
	for(obj_node = p_on_sectors->GetNext(); obj_node; obj_node = next)
	{
		next = obj_node->GetNext();
		p_sector = obj_node->GetData();

		p_sector->set_flags(CSector::mADD_TO_SUPER_SECTORS);
		AddSector(p_sector);
		p_sector->SetActive(true);
		//Dbg_Message("Added sector %x in toggle", p_sector->GetChecksum());
	}

	// And update the SuperSectors
	UpdateSuperSectors(p_off_sectors);

	return m_using_add_sectors;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 			CScene::CloneSector(uint32 orig_sector_checksum, CScene *p_dest_scene, bool instance, bool add_to_super_sectors)
{
	CSector *p_orig_sector = GetSector(orig_sector_checksum);

	if (p_orig_sector)
	{
		return CloneSector(p_orig_sector, p_dest_scene, instance, add_to_super_sectors);
	} else {
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 			CScene::CloneSector(CSector *p_orig_sector, CScene *p_dest_scene, bool instance, bool add_to_super_sectors)
{
	// Use current scene if dest is NULL
	if (!p_dest_scene)
	{
		p_dest_scene = this;
	}

	CSector *p_new_sector = p_orig_sector->clone(instance, add_to_super_sectors, p_dest_scene);

	if (p_new_sector)
	{
		p_dest_scene->AddSector(p_new_sector);

		return p_new_sector->GetChecksum();
	} else {
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool 			CScene::DeleteSector(uint32 sector_checksum)
{
	CSector *p_sector = GetSector(sector_checksum);

	if (p_sector)
	{
		return DeleteSector(p_sector);
	} else {
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool 			CScene::DeleteSector(CSector *p_sector)
{
	if (p_sector->get_flags() & CSector::mIN_SUPER_SECTORS)
	{
		p_sector->set_flags(CSector::mMARKED_FOR_DELETION);		// Can't delete, yet

		return false;
	} else {
		if (GetSector(p_sector->GetChecksum()) == p_sector)		// Just in case we're doing this with AddSectors
		{
			mp_sector_table->FlushItem(p_sector->GetChecksum());
		}

		delete p_sector;

		return true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CScene::PostLoad(const char *p_name)
{
	strcpy(m_scene_filename, p_name);	// needs name for Pip::Unload()

	plat_post_load();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CScene::PostAdd(const char *p_name, Nx::CTexDict * p_tex_dict)
{
	strcpy(m_add_scene_filename, p_name);	// needs to store name somewhere for Pip::Unload(), if we use Pip
	mp_add_tex_dict = p_tex_dict; 
	m_using_add_sectors = true;

	plat_post_add();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::read_collision(const char *p_name, char *p_pip_name, int &num_coll_sectors,
									   CCollStaticTri * &p_coll_sectors, CCollObjTriData * &p_coll_sector_data, Mth::CBBox &bbox, bool is_net)
{
	// for now collision is kind of assumed to be platform independent 
	sprintf(p_pip_name,"levels\\%s\\%s%s.col.%s",p_name,p_name,is_net?"_net":"",CEngine::sGetPlatformExtension());

	Dbg_Message ( "Loading collision %s....", p_pip_name );
	
	bool	found_point = false;

	uint8 *p_base_addr = (uint8 *) Pip::Load(p_pip_name);
	if (p_base_addr)
	{
		Nx::CCollObjTriData::SReadHeader *p_header = (Nx::CCollObjTriData::SReadHeader *) p_base_addr;
		p_base_addr += sizeof(Nx::CCollObjTriData::SReadHeader);

#ifndef __PLAT_NGC__
		Dbg_Message ( "Version # %d header sizeof %d", p_header->m_version, sizeof(Nx::CCollObjTriData));
		Dbg_Message ( "Number of objects: %d verts: %d faces: %d", p_header->m_num_objects, p_header->m_total_num_verts, p_header->m_total_num_faces_large + p_header->m_total_num_faces_small);
		Dbg_Message ( "Small verts: %d Large verts: %d", p_header->m_total_num_verts_small, p_header->m_total_num_verts_large);
#endif		// __PLAT_NGC__
		Dbg_MsgAssert(p_header->m_version >= 9, ("Collision version must be at least 9."));

		// reserve space for objects
		num_coll_sectors = p_header->m_num_objects;
		p_coll_sector_data = (Nx::CCollObjTriData *) p_base_addr; //new Cld::CCollSector [m_num_coll_sectors];

		// Calculate base addresses for vert and face arrays
		uint8 *p_base_vert_addr = (uint8 *) (p_coll_sector_data + num_coll_sectors);
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
		uint8 *p_base_intensity_addr = NULL;
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

		// Reserve space for collsion objects
		p_coll_sectors = new CCollStaticTri[p_header->m_num_objects];

		// Read objects
		for (int oidx = 0; oidx < p_header->m_num_objects; oidx++)
		{
			p_coll_sector_data[oidx].InitCollObjTriData(this, p_base_vert_addr, p_base_intensity_addr, p_base_face_addr,
														p_base_node_addr, p_base_face_idx_addr);
			p_coll_sector_data[oidx].InitBSPTree();

			p_coll_sectors[oidx].SetGeometry(&(p_coll_sector_data[oidx]));

			if (p_coll_sector_data[oidx].GetNumFaces() > 0)	 // only add bbox if there are some faces in the object.....
			{
				// Add to scene bbox
				bbox.AddPoint(p_coll_sector_data[oidx].GetBBox().GetMin());
				bbox.AddPoint(p_coll_sector_data[oidx].GetBBox().GetMax());
				found_point = true;
			}
		}
		if (!found_point)
		{
			// if there was no collision, then set up a dummy 200 inch bounding box at the origin
			bbox.AddPoint(Mth::Vector (-100,-100,-100));
			bbox.AddPoint(Mth::Vector (100,100,100));
		}
	} 
	else 
	{
		Dbg_Error ( "Could not open collision file\n" );
		return false;
	}

	Dbg_Message ( "successfully read collision" );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::LoadCollision(const char *p_name, bool is_net)
{
	read_collision(p_name, m_coll_filename, m_num_coll_sectors, mp_coll_sectors, mp_coll_sector_data, m_collision_bbox, is_net);

	Dbg_Message("Scene bounding box: min (%f, %f, %f) max (%f, %f, %f)", 
				m_collision_bbox.GetMin()[X], m_collision_bbox.GetMin()[Y], m_collision_bbox.GetMin()[Z], 
				m_collision_bbox.GetMax()[X], m_collision_bbox.GetMax()[Y], m_collision_bbox.GetMax()[Z]);

	if (m_num_coll_sectors > 0)
	{
		// Create super sectors
		if (m_in_super_sectors)
		{
			mp_sector_man = new SSec::Manager;
			mp_sector_man->GenerateSuperSectors(m_collision_bbox);

			mp_sector_man->AddCollisionToSuperSectors(mp_coll_sectors, m_num_coll_sectors);
		}

		// Add to CSectors
		for (int i = 0; i < m_num_coll_sectors; i++)
		{
			CSector *p_sector = GetSector(mp_coll_sectors[i].GetChecksum());
			if (!p_sector)	// Don't assert now since there may not be renderable data
			{
				// Collision info, but not renderable
				// so, we still need to create a Sector, but with no renderable component
				p_sector = CreateSector(); 			// create empty secotr
				p_sector->SetChecksum(mp_coll_sectors[i].GetChecksum());
				p_sector->SetActive(true);
				AddSector(p_sector);
			}
			p_sector->AddCollSector(&(mp_coll_sectors[i]));
			p_sector->set_flags(CSector::mIN_SUPER_SECTORS);
		}

	}

	CEngine::sGetWeather()->UpdateGrid();

	return plat_load_collision(p_name);	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::AddCollision(const char *p_name)
{
	Mth::CBBox			add_collision_bbox;

	if (mp_coll_sectors == NULL)
	{
		return false;			// collision not needed
	}

	read_collision(p_name, m_add_coll_filename, m_num_add_coll_sectors, mp_add_coll_sectors, mp_add_coll_sector_data, add_collision_bbox);

	if ((add_collision_bbox.GetMin()[X] < m_collision_bbox.GetMin()[X]) ||
		(add_collision_bbox.GetMax()[X] > m_collision_bbox.GetMax()[X]) ||
		(add_collision_bbox.GetMin()[Z] < m_collision_bbox.GetMin()[Z]) ||
		(add_collision_bbox.GetMax()[Z] > m_collision_bbox.GetMax()[Z]))
	{
		Dbg_Message("Add Scene bounding box: min (%f, %f, %f) max (%f, %f, %f)", 
					add_collision_bbox.GetMin()[X], add_collision_bbox.GetMin()[Y], add_collision_bbox.GetMin()[Z], 
					add_collision_bbox.GetMax()[X], add_collision_bbox.GetMax()[Y], add_collision_bbox.GetMax()[Z]);
		Dbg_Message("Scene bounding box: min (%f, %f, %f) max (%f, %f, %f)", 
					m_collision_bbox.GetMin()[X], m_collision_bbox.GetMin()[Y], m_collision_bbox.GetMin()[Z], 
					m_collision_bbox.GetMax()[X], m_collision_bbox.GetMax()[Y], m_collision_bbox.GetMax()[Z]);
		Dbg_MsgAssert(add_collision_bbox.Within(m_collision_bbox), ("Can't add collision outside of original scene"));
	}

	if (m_num_add_coll_sectors > 0)
	{
		// Add to CSectors
		for (int i = 0; i < m_num_add_coll_sectors; i++)
		{
			CSector *p_sector = GetSector(mp_add_coll_sectors[i].GetChecksum());
			if (!p_sector)	// Don't assert now since there may not be renderable data
			{
				Dbg_Message("We don't necessarily want to be creating invisible sectors on an incremental update");

				// Collision info, but not renderable
				// so, we still need to create a Sector, but with no renderable component
				p_sector = CreateSector(); 			// create empty secotr
				p_sector->SetChecksum(mp_add_coll_sectors[i].GetChecksum());
				p_sector->SetActive(true);
				p_sector->set_flags(CSector::mADD_TO_SUPER_SECTORS);
				AddSector(p_sector);

				Lst::Node<CSector> *node = new Lst::Node<CSector>(p_sector);
				mp_add_sectors->AddToTail(node);
			}
			p_sector->AddCollSector(&(mp_add_coll_sectors[i]));
			Dbg_Assert(p_sector->get_flags() & CSector::mADD_TO_SUPER_SECTORS);
		}

		// Now add to SuperSectors
		UpdateSuperSectors(mp_orig_sectors);
	}

	return plat_add_collision(p_name);	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::CreateCollision(const Mth::CBBox & scene_bbox)
{
	// Make sure we asked for it at scene creation
	if (!m_in_super_sectors)
	{
		return false;
	}

	m_collision_bbox = scene_bbox;
	mp_sector_man = new SSec::Manager;
	mp_sector_man->GenerateSuperSectors(m_collision_bbox);

	return true;
}

#ifdef __NOPT_ASSERT__
void			CScene::DebugRenderCollision(uint32 ignore_1, uint32 ignore_0)
{
	if (Nx::CEngine::GetWireframeMode() == 4 || Nx::CEngine::GetWireframeMode() == 5)
	{
	
		#ifdef	__PLAT_NGPS__
		#if 1
		
		// Traverse the tree at the mesh level
		// so we can see the actual meshes
		
		CPs2Scene * p_ps2_scene = (CPs2Scene *) Nx::CEngine::sGetMainScene();
		p_ps2_scene->GetEngineScene()->RenderWireframe(Nx::CEngine::GetWireframeMode());

		
			 
		#else	 
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
		Mth::Vector	*p_verts = new Mth::Vector[100000];
		Mem::Manager::sHandle().PopContext();
		
		
		Nx::CScene	*p_scene = Nx::CEngine::sGetMainScene();
		// crappy wireframe rendering of renderable, so we can see if there is any invislbe junk there
		Lst::HashTable< Nx::CSector > * p_sector_list = p_scene->GetSectorList();
		
		int sectors = 0;
		int vert_count=0;
		int lines = 0;
		if (p_sector_list)
		{
			p_sector_list->IterateStart();	
			Nx::CSector *p_sector = p_sector_list->IterateNext();		
			while(p_sector && lines <100000)   // Can't draw too many lines in this mode for some reason
			{
	
				if (p_sector->IsActive())
				{
			
							 
					Nx::CGeom 	*p_geom = p_sector->GetGeom();
					if (p_geom)
					{
					
						NxPs2::CGeomNode *p_node = (((CPs2Geom*)p_geom)->GetEngineObject());
						if (p_node->WasRendered())
						{
							sectors++;
						
							//
							// Do renderable geometry
							int verts = p_geom->GetNumRenderVerts();
							if ( verts > 3000)
							{
							//	printf ("Strange no of verts %d on %s\n",verts, Script::FindChecksumName(p_sector->GetChecksum()));
							}
							if (verts < 30000)
							{
								//NxPs2::BeginLines3D(0x80000000 + (0x00ff00ff));		// Magenta 
								
								vert_count += verts;  
								  
								#define	peak 500
								int n=p_geom->GetNumRenderPolys();
								int r,g,b;
								r=g=b=0;
								if (n <= peak )
								{
									r = (255 * (n)) / peak;  	// r ramps up (black to red)	
								}
								else if (n <= peak * 2)
								{
									r = 255;
									b = (255 * (n - peak) / (peak));	// b&g ramps up (to white)
									g = b;
									
								}
								else
								{
									r = g = b = 255;
								}
								uint32 rgb = (b<<16)|(g<<8)|r;	

								
								if (Nx::CEngine::GetWireframeMode() == 4)
								{
									NxPs2::BeginLines3D(0x80000000 + (0x00ffffff & p_sector->GetChecksum()));		
								}
								else
								{
									NxPs2::BeginLines3D(0x80000000 + (0x00ffffff & rgb));		
								}
								
								p_geom->GetRenderVerts(p_verts);
								Mth::Vector *p_vert = p_verts+1;
								for (int i = 1; i < verts; i++)
								{
									NxPs2::DrawLine3D((*p_vert)[X],(*p_vert)[Y],(*p_vert)[Z],p_vert[-1][X],p_vert[-1][Y],p_vert[-1][Z]);								
									p_vert++;
									lines++;
								} // end for
						
								NxPs2::EndLines3D();
							} // end if
						}						
					}
				}
				p_sector = p_sector_list->IterateNext();
			}
		}
//		printf ("Renderable wireframe %d sectors, %d verts, %d lines\n",sectors, vert_count, lines);
		delete [] p_verts;
		#endif
		#endif
	}	
	else
	{
		if (!m_in_super_sectors)
		{
			// if collision data is not being used, then just return
			return;
		}
	
		if (mp_coll_sectors)
		{
			for (int i = 0; i < m_num_coll_sectors; i++)
			{
				mp_coll_sectors[i].DebugRender(ignore_1,ignore_0);
			}
		} else {
			// Try finding the collision through the sectors
			mp_sector_table->IterateStart();
			CSector *p_sector;
			while ((p_sector = mp_sector_table->IterateNext()))
			{
				if (p_sector->GetCollSector())	  // Might not have it yet in park editor
				{
					p_sector->GetCollSector()->DebugRender(ignore_1, ignore_0);
				}
			}
		}
	}
}
#endif

void			CScene::DebugCheckForHoles()
{

	for (int i = 0; i < m_num_coll_sectors; i++)
	{
		printf ("%4d/%d ",i,m_num_coll_sectors);
		CCollStaticTri *p_static_tri = static_cast<CCollStaticTri *>(&(mp_coll_sectors[i]));
		Dbg_Assert(p_static_tri);
		p_static_tri->CheckForHoles();
	}

}


// Return the number of collision sectors in the scene
int		CScene::GetNumCollSectors()
{
	return m_num_coll_sectors;
}

// return a pointer to static collision data
// for a collision sector
// used
CCollStaticTri * CScene::GetCollStatic(int n)
{
	return &(mp_coll_sectors[n]);	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CScene::UpdateSuperSectors(Lst::Head<CSector> *p_additional_remove_list)
{
	// Check if we even have SuperSectors
	if (!mp_sector_man)
	{
		return;
	}

	Lst::Head< Nx::CCollStatic > coll_add_list;
	Lst::Head< Nx::CCollStatic > coll_remove_list;
	Lst::Head< Nx::CCollStatic > coll_update_list;
	Lst::Head< CSector > 	  sector_delete_list;

	/*
	Mem::Heap *p_top_heap = Mem::Manager::sHandle().TopDownHeap();
	Mem::Region *p_top_region = p_top_heap->mp_region;
	Ryan("UpdateSuperSectors()\n");
	Ryan("  free %d\n", p_top_heap->mFreeMem.m_count + p_top_region->MemAvailable());
	Ryan("  used %d\n", p_top_heap->mUsedMem.m_count);
	Ryan("  largest %d\n", p_top_heap->LargestFreeBlock());
	*/

	// Allocate temp list on high heap
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

	mp_sector_table->IterateStart();

	Lst::Node< Nx::CCollStatic > *node;
	CSector *p_sector;

	// Make Add and remove list
	while ((p_sector = mp_sector_table->IterateNext()))
	{
		// Add List
		if (p_sector->get_flags() & CSector::mADD_TO_SUPER_SECTORS)
		{
			node = new Lst::Node< Nx::CCollStatic > ( p_sector->GetCollSector() );
			coll_add_list.AddToTail(node);
			p_sector->clear_flags(CSector::mADD_TO_SUPER_SECTORS);
			p_sector->set_flags(CSector::mIN_SUPER_SECTORS);

			//Dbg_Message("Adding sector %x and collision %x of checksum %x", p_sector, p_sector->GetCollSector(), p_sector->GetChecksum());
			//Mth::CBBox b_box = p_sector->GetCollSector()->GetGeometry()->GetBBox();
			//Dbg_Message("Adding sector %x and collision %x and bounding box [%f,%f,%f]-[%f,%f,%f]", p_sector->GetChecksum(), p_sector->GetCollSector()->GetChecksum(),
			//			   b_box.GetMin()[X], b_box.GetMin()[Y], b_box.GetMin()[Z],
			//			   b_box.GetMax()[X], b_box.GetMax()[Y], b_box.GetMax()[Z]);

			Dbg_MsgAssert(!(p_sector->get_flags() & CSector::mINVALID_SUPER_SECTORS),
						  ("Trying to add collision that needed updating"));
		}

		// Remove List
		if (p_sector->get_flags() & (CSector::mMARKED_FOR_DELETION | CSector::mREMOVE_FROM_SUPER_SECTORS))
		{
			node = new Lst::Node< Nx::CCollStatic > ( p_sector->GetCollSector() );
			coll_remove_list.AddToTail(node);

			// Also make a sector delete list
			if (p_sector->get_flags() & CSector::mMARKED_FOR_DELETION)
			{
				Lst::Node< CSector > *sec_node = new Lst::Node< CSector > (p_sector);
				sector_delete_list.AddToTail(sec_node);
			} else {
				p_sector->clear_flags(CSector::mREMOVE_FROM_SUPER_SECTORS);
			}

			//Dbg_Message("Deleting sector %x and collision %x of checksum %x", p_sector, p_sector->GetCollSector(), p_sector->GetChecksum());
			//Mth::CBBox b_box = p_sector->GetCollSector()->GetGeometry()->GetBBox();
			//Dbg_Message("Deleting sector %x and collision %x and bounding box [%f,%f,%f]-[%f,%f,%f]", p_sector->GetChecksum(), p_sector->GetCollSector()->GetChecksum(),
			//				b_box.GetMin()[X], b_box.GetMin()[Y], b_box.GetMin()[Z],
			//				b_box.GetMax()[X], b_box.GetMax()[Y], b_box.GetMax()[Z]);
			Dbg_MsgAssert(!(p_sector->get_flags() & CSector::mADD_TO_SUPER_SECTORS),
						  ("Collision being both added and deleted from SuperSectors"));
			Dbg_MsgAssert(p_sector->get_flags() & CSector::mIN_SUPER_SECTORS,
						  ("Trying to remove sector from SuperSector that isn't in any SuperSector"));

			Dbg_MsgAssert(!(p_sector->get_flags() & CSector::mINVALID_SUPER_SECTORS),
						  ("Trying to remove collision that hasn't been updated"));
		}

		// Update List
		if (p_sector->get_flags() & CSector::mINVALID_SUPER_SECTORS)
		{
			node = new Lst::Node< Nx::CCollStatic > ( p_sector->GetCollSector() );
			coll_update_list.AddToTail(node);
			p_sector->clear_flags(CSector::mINVALID_SUPER_SECTORS);

			//Mth::CBBox b_box = p_sector->GetCollSector()->GetGeometry()->GetBBox();
			//Dbg_Message("Updating sector %x and collision %x and bounding box [%f,%f,%f]-[%f,%f,%f]", p_sector->GetChecksum(), p_sector->GetCollSector()->GetChecksum(),
			//			   b_box.GetMin()[X], b_box.GetMin()[Y], b_box.GetMin()[Z],
			//			   b_box.GetMax()[X], b_box.GetMax()[Y], b_box.GetMax()[Z]);

			Dbg_MsgAssert(p_sector->get_flags() & CSector::mIN_SUPER_SECTORS,
						  ("Collision being updated that isn't in SuperSectors"));
		}
	}

	// Check additional remove list
	if (p_additional_remove_list)
	{
		Lst::Node< CSector > *obj_node, *next;

		for(obj_node = p_additional_remove_list->GetNext(); obj_node; obj_node = next)
		{
			next = obj_node->GetNext();
			p_sector = obj_node->GetData();

			Dbg_Assert(!(p_sector->get_flags() & CSector::mMARKED_FOR_DELETION));		// Make sure it shouldn't be deleted

			// Make sure it isn't already on the list
			bool new_node = true;
			if(coll_remove_list.CountItems() > 0)
			{
				Lst::Node<Nx::CCollStatic> *node_coll, *node_next;
				for(node_coll = coll_remove_list.GetNext(); node_coll; node_coll = node_next)
				{
					node_next = node_coll->GetNext();
					if (node_coll->GetData() == p_sector->GetCollSector())
					{
						new_node = false;
						break;
					}

				}
			}

			if (new_node)
			{
				node = new Lst::Node< Nx::CCollStatic > ( p_sector->GetCollSector() );
				coll_remove_list.AddToTail(node);
				p_sector->clear_flags(CSector::mREMOVE_FROM_SUPER_SECTORS);
				//Dbg_Message("Deleting additional sector %x and collision %x of checksum %x", p_sector, p_sector->GetCollSector(), p_sector->GetChecksum());
				//Dbg_Message("Deleting additional sector %x and collision %x", p_sector->GetChecksum(), p_sector->GetCollSector()->GetChecksum());
			}
		}
	}

	Mem::Manager::sHandle().PopContext();

	// Do the actual update
	mp_sector_man->UpdateCollisionSuperSectors(coll_add_list, coll_remove_list, coll_update_list);

	Lst::Node<Nx::CCollStatic> *node_coll, *node_next;

	//
	// Free lists
	//
	if(coll_add_list.CountItems() > 0)
	{
		for(node_coll = coll_add_list.GetNext(); node_coll; node_coll = node_next)
		{
			node_next = node_coll->GetNext();

			delete node_coll;
		}
	}

	if(coll_remove_list.CountItems() > 0)
	{
		for(node_coll = coll_remove_list.GetNext(); node_coll; node_coll = node_next)
		{
			node_next = node_coll->GetNext();

			delete node_coll;
		}
	}

	if(coll_update_list.CountItems() > 0)
	{
		for(node_coll = coll_update_list.GetNext(); node_coll; node_coll = node_next)
		{
			node_next = node_coll->GetNext();

			delete node_coll;
		}
	}

	// And finally delete the marked sectors
	if(sector_delete_list.CountItems() > 0)
	{
		Lst::Node<CSector> *node_sector, *node_sec_next;
		for(node_sector = sector_delete_list.GetNext(); node_sector; node_sector = node_sec_next)
		{
			node_sec_next = node_sector->GetNext();

			p_sector = node_sector->GetData();

			mp_sector_table->FlushItem(p_sector->GetChecksum());

			p_sector->clear_flags(CSector::mIN_SUPER_SECTORS);
			delete p_sector;

			delete node_sector;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CScene::ClearSuperSectors()
{
	// Check if we even have SuperSectors
	if (!mp_sector_man)
	{
		return;
	}

	mp_sector_table->IterateStart();

	CSector *p_sector;

	// Check all sectors to make sure they need deleting
	while ((p_sector = mp_sector_table->IterateNext()))
	{
		// Remove List
		if (p_sector->get_flags() & CSector::mMARKED_FOR_DELETION)
		{
			//Dbg_Message("Deleting sector %x and collision %x of checksum %x", p_sector, p_sector->GetCollSector(), p_sector->GetChecksum());
			Dbg_MsgAssert(!(p_sector->get_flags() & CSector::mADD_TO_SUPER_SECTORS),
						  ("Collision being both added and deleted from SuperSectors"));
			Dbg_MsgAssert(p_sector->get_flags() & CSector::mIN_SUPER_SECTORS,
						  ("Trying to remove sector from SuperSector that isn't in any SuperSector"));

			Dbg_MsgAssert(!(p_sector->get_flags() & CSector::mINVALID_SUPER_SECTORS),
						  ("Trying to remove collision that hasn't been updated"));

			p_sector->clear_flags(CSector::mIN_SUPER_SECTORS);
			delete p_sector;
		} else {
			Dbg_MsgAssert(0, ("Trying to clear SuperSectors even though sector %x hasn't been marked for deletion", p_sector->GetChecksum()));
		}
	}

	// Do the actual clear
	mp_sector_man->ClearCollisionSuperSectors();

	// And clear the hash table
	mp_sector_table->FlushAllItems();
}

// Given a bounding box, then set the active flag of those 
// sectors that intersect this box by at least a certain abount 
void CScene::SetActiveInBox(Mth::CBBox &box, bool active, float min_intersect)
{
	
	Mth::CBBox smaller_box = box;
	
	// later..
//	smaller_box.Shrink(min_intersect);   
	
	mp_sector_table->IterateStart();
	CSector *p_sector;
	while ((p_sector = mp_sector_table->IterateNext()))
	{
//		printf ("Checking sector\n");
		if (smaller_box.Intersect(p_sector->GetBoundingBox()))
		{
//			printf("Setting active = %d\n",active);
			p_sector->SetActive(active);
		}
	}

}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Create an empty sector
CSector	*			CScene::CreateSector()
{
	CSector	*p_sector = plat_create_sector();
	p_sector->mp_geom = NULL;
	p_sector->mp_coll_sector = NULL;
	return p_sector;
}


void	CScene::GetMetrics(Script::CStruct * p_info)
{
	p_info->AddString(CRCD(0xc3f4169a,"FileName"),GetSceneFilename());

	int total_sectors = 0;
	int render_sectors = 0;	
	int render_verts = 0;	
	int render_polys = 0;	
	int render_base_polys = 0;	
	// Iterate over the sectors, counting verious things	
	Lst::HashTable< Nx::CSector > * p_sector_list = GetSectorList();
	if (p_sector_list)
	{
		p_sector_list->IterateStart();	
		Nx::CSector *p_sector = p_sector_list->IterateNext();		
		while(p_sector)
		{
			total_sectors++;
			Nx::CGeom 	*p_geom = p_sector->GetGeom();
			if (p_geom)
			{
				// Do renderable geometry
				render_sectors++;
				render_verts += p_geom->GetNumRenderVerts();
				render_polys += p_geom->GetNumRenderPolys();
				render_base_polys += p_geom->GetNumRenderBasePolys();
			}
			p_sector = p_sector_list->IterateNext();
		}
	}
	
	p_info->AddInteger(CRCD(0x4a6bf967,"Sectors"),total_sectors);
	p_info->AddInteger(CRCD(0xac30a9d,"ColSectors"),GetNumCollSectors());
	p_info->AddInteger(CRCD(0x969d3af6,"Verts"),render_verts);
	p_info->AddInteger(CRCD(0xd576df05,"Polys"),render_polys);
	p_info->AddInteger(CRCD(0x1e3061ee,"BasePolys"),render_base_polys);

	p_info->AddInteger(CRCD(0x5714c480,"TextureMemory"),GetTexDict()->GetFileSize());
	
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CScene::SetMajorityColor(Image::RGBA rgba)
{
	plat_set_majority_color(rgba);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA	CScene::GetMajorityColor() const
{
	return plat_get_majority_color();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CScene::plat_post_load()
{
	printf ("STUB: PostLoad\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CScene::plat_post_add()
{
	printf ("STUB: PostAdd\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::plat_load_textures(const char *p_name)
{
	printf ("STUB: LoadTextures\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::plat_load_collision(const char *p_name)
{
	printf ("STUB: LoadCollision\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::plat_add_collision(const char *p_name)
{
	printf ("STUB: AddCollision\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CScene::plat_unload_add_scene()
{
	printf ("STUB: UnloadAddScene\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSector*			CScene::plat_create_sector()
{
	printf ("STUB: plat_create_sector\n");
	return NULL;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CScene::plat_set_majority_color(Image::RGBA rgba)
{
	// STUB: only really needed on PS2, so ignorable on other platforms
}
		 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA			CScene::plat_get_majority_color() const
{
	// STUB: only really needed on PS2, so ignorable on other platforms
	return Image::RGBA(0x80, 0x80, 0x80, 0x80);
}




} // namespace Nx




