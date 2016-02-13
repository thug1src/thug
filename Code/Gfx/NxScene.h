///////////////////////////////////////////////////////////////////////////////////
// NXSCENE.H - Neversoft Engine, Rendering portion, Platform independent interface

#ifndef	__GFX_NX_SCENE_H__
#define	__GFX_NX_SCENE_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/hashtable.h>

#include <sk/engine/supersector.h>
#include 	<gfx/image/imagebasic.h>


namespace Script
{
	class	CStruct;
}

namespace Nx
{
/////////////////////////////////////////////////////////////////////////
// Forward declarations
class	CSector;
class	CTexDict;
class	CCollStaticTri;
class	CCollObjTriData;

////////////////////////////////////////////////////////////////////////
// CScene:
//
// Holds all the CSectors for a scene.
//

class		CScene
{
public:
								CScene( int sector_table_size = 10 );
	virtual						~CScene();

	void						PostLoad(const char *p_name);
	void						PostAdd(const char *p_name, Nx::CTexDict * p_tex_dict);
	bool						LoadTextures(const char *p_name);			// load scene textures
	bool						LoadCollision(const char *p_name, bool is_net=false);			// load scene collision data
	bool						AddCollision(const char *p_name);			// add scene collision data
	bool						CreateCollision(const Mth::CBBox & scene_bbox); // load scene collision data

	bool						ToggleAddScene();							// Toggle the orig and add sectors
	bool						UnloadAddScene();							// unloads previous add scene

	bool						InSuperSectors() const;
	void						SetInSuperSectors(bool);
	bool						IsSky() const;
	void						SetIsSky(bool);

	CSector	*					GetSector(uint32 sector_checksum);			// get a sector pointer, based on the checksum of the name
	void						AddSector(CSector *pSector);				// Add a sector to the list
	Lst::HashTable<CSector> *	GetSectorList() const;			  			// get the whole list

	// Replacing a sector is for in-house testing.  Since PIP'ed CSectors can't be deleted individually,
	// the replace will hide the old one.
	CSector *					ReplaceSector(CSector *pSector);			// returns old sector, if any

	// Cloning copies a sector, puts it in hash table, and returns its checksum (p_dest_scene of NULL uses current scene)
	uint32						CloneSector(uint32 orig_sector_checksum, CScene *p_dest_scene = NULL,
											bool instance = false, bool add_to_super_sectors = true);
	uint32						CloneSector(CSector *p_orig_sector, CScene *p_dest_scene = NULL,
											bool instance = false, bool add_to_super_sectors = true);

	// creates an empty sector
	CSector	*					CreateSector();
	
	
	// Deletes a sector and takes it off the list
	bool						DeleteSector(uint32 sector_checksum);
	bool						DeleteSector(CSector *p_sector);

	// Updates the SuperSectors with any clone or quickupdate changes
	void						UpdateSuperSectors(Lst::Head<CSector> *p_additional_remove_list = NULL);
	void						ClearSuperSectors();						// note ALL sectors must already be marked for deletion

	// allow access to the collision sectors												  
	int							GetNumCollSectors();
	CCollStaticTri * 			GetCollStatic(int n);


	// Sets sectors active or inactive if they are inside a particular bounding box																								  
	void 						SetActiveInBox(Mth::CBBox &box, bool active, float min_intersect);
	
	const Mth::CBBox &			GetRenderBoundingBox() const;				// get bounding box of scene renderables
	const Mth::CBBox &			GetCollisionBoundingBox() const;			// get bounding box of scene collision
	SSec::Manager *				GetSuperSectorManager() const;				// get the super sectors, if any

	void						SetTexDict(CTexDict * p_tex_dict);			// set texture dictionary
	CTexDict *					GetTexDict() const;							// set texture dictionary

	void						SetID(uint32 id);							// Set name checksum ID
	uint32						GetID() const;
	const char *				GetSceneFilename() const;
	const char *				GetAddSceneFilename() const;

	void						DebugRenderCollision(uint32 ignore_1, uint32 ignore_0);						// render in wireframe
	void						DebugCheckForHoles();

	void						GetMetrics(Script::CStruct * p_info);

	void						SetMajorityColor(Image::RGBA rgba);
	Image::RGBA					GetMajorityColor() const;

protected:
	// read scene collision file
	bool						read_collision(const char *p_name, char *p_pip_name, int &num_coll_sectors,
											   CCollStaticTri * &p_coll_sectors, CCollObjTriData * &p_coll_sector_data,
											   Mth::CBBox &bbox, bool is_net=false);

	void						remove_sector_from_table(uint32 sector_checksum);

	char						m_scene_filename[128];						// scene filename (kept around for unload)
	uint32						m_id;										// checksum of name
	
	CTexDict *					mp_tex_dict;								// texture dictionary that this scene uses	
	
	bool						m_in_super_sectors;							// Is this scene in the super sectors
	bool						m_sky;										// Tells if scene is a sky type

	Mth::CBBox					m_render_bbox;								// scene renderable bounding box
	Mth::CBBox					m_collision_bbox;							// scene collision bounding box
	Lst::HashTable< CSector > *	mp_sector_table;							// All the CSector pointers
	SSec::Manager *				mp_sector_man;								// SuperSector manager

	char						m_coll_filename[128];						// collision filename (kept around for unload)
	int							m_num_coll_sectors;							// non-cloned collision
	CCollStaticTri *			mp_coll_sectors;							// Static collision objects
	CCollObjTriData *			mp_coll_sector_data;						// Static collision object data

	// For the incremental update code
	char						m_add_scene_filename[128];
	CTexDict *					mp_add_tex_dict; 
	char						m_add_coll_filename[128];
	int							m_num_add_coll_sectors;
	CCollStaticTri *			mp_add_coll_sectors;
	CCollObjTriData *			mp_add_coll_sector_data;

	bool						m_using_add_sectors;
	Lst::Head < CSector > *		mp_orig_sectors;
	Lst::Head < CSector > *		mp_add_sectors;

private:
	////////////////////////////////////////////////////////////
	// Platform specific function calls	
	//
	virtual void				plat_post_load();
	virtual void				plat_post_add();
	virtual bool				plat_load_textures(const char *p_name);		// load textures 
	virtual bool				plat_load_collision(const char *p_name);	// load collision data
	virtual bool				plat_add_collision(const char *p_name);		// add collision data
	virtual bool				plat_unload_add_scene();					// unloads previous add scene
	virtual CSector	*			plat_create_sector();  						// create empty sector
	virtual void				plat_set_majority_color(Image::RGBA rgba);	// set the most common sector color
	virtual Image::RGBA			plat_get_majority_color() const;			// get the most common sector color
};

///////////////////////////////////////////////////////////////////////
//
// Inline functions
//

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Lst::HashTable< CSector > * CScene::GetSectorList() const
{
	return mp_sector_table;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					CScene::InSuperSectors() const
{
	return m_in_super_sectors;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CScene::SetInSuperSectors(bool s)
{
	m_in_super_sectors = s;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					CScene::IsSky() const
{
	return m_sky;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CScene::SetIsSky(bool s)
{
	m_sky = s;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::CBBox &	CScene::GetRenderBoundingBox() const
{
	return m_render_bbox;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::CBBox &	CScene::GetCollisionBoundingBox() const
{
	return m_collision_bbox;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline SSec::Manager *		CScene::GetSuperSectorManager() const
{
	return mp_sector_man;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline 	void				CScene::SetTexDict(CTexDict * p_tex_dict)
{
	mp_tex_dict = p_tex_dict;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	CTexDict *			CScene::GetTexDict() const
{
	return mp_tex_dict;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline 	void				CScene::SetID(uint32 id)
{
	m_id = id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	uint32				CScene::GetID() const
{
	return m_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	const char *		CScene::GetSceneFilename() const
{
	return m_scene_filename;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	const char *		CScene::GetAddSceneFilename() const
{
	return m_add_scene_filename;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline void					CScene::remove_sector_from_table(uint32 sector_checksum)
{
	mp_sector_table->FlushItem(sector_checksum);
}

}


#endif

