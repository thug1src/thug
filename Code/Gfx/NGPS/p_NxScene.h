///////////////////////////////////////////////////////////////////////////////
// p_NxScene.h

#ifndef	__GFX_P_NX_SCENE_H__
#define	__GFX_P_NX_SCENE_H__

#include 	"Gfx/nxscene.h"

namespace NxPs2
{
	class CGeomNode;
}

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CScene
class	CPs2Scene : public CScene
{
public:
								CPs2Scene();
	virtual						~CPs2Scene();

	NxPs2::CGeomNode *			GetEngineScene() const;
	void						SetEngineScene(NxPs2::CGeomNode *p_scene);

	NxPs2::CGeomNode *			GetEngineAddScene() const;
	void						SetEngineAddScene(NxPs2::CGeomNode *p_scene);

	NxPs2::CGeomNode *			GetEngineCloneScene() const;
	void						SetEngineCloneScene(NxPs2::CGeomNode *p_scene);

private:		
	virtual void				plat_post_load();	
	virtual void				plat_post_add();	
	virtual bool				plat_load_textures(const char *p_name);		// load textures 
	virtual bool				plat_load_collision(const char *p_name);	// load collision data
	virtual bool				plat_add_collision(const char *p_name);		// add collision data
	virtual bool				plat_unload_add_scene();					// unloads previous add scene
	virtual	CSector	*			plat_create_sector();	 					// empty sector
	virtual void				plat_set_majority_color(Image::RGBA rgba);	// set the most common sector color
	virtual Image::RGBA			plat_get_majority_color() const;			// get the most common sector color
	

	void						create_sectors(NxPs2::CGeomNode *p_node);
	void						add_sectors(NxPs2::CGeomNode *p_node);
	void						delete_sectors(NxPs2::CGeomNode *p_node);

	NxPs2::CGeomNode *			mp_plat_scene;		// Platform-dependent data
	NxPs2::CGeomNode *			mp_plat_clone_scene;
	NxPs2::CGeomNode *			mp_plat_add_scene;
};

} // Namespace Nx  			

#endif
