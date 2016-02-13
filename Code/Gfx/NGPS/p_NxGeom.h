//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxGeom.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  3/5/2002
//****************************************************************************

#ifndef	__GFX_P_NX_GEOM_H__
#define	__GFX_P_NX_GEOM_H__
    
#include "core/math.h"

#include "gfx/nxgeom.h"

namespace NxPs2
{
	class CInstance;
	class CGeomNode;
}
						   
namespace Nx
{
	class CPs2Mesh;
	class CModelLights;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/////////////////////////////////////////////////////////////////////////////////////
//
// Here's a machine specific implementation of the CGeom
    
class CPs2Geom : public CGeom
{
                                      
public:
						CPs2Geom();
	virtual 			~CPs2Geom();

	void				SetEngineObject(NxPs2::CGeomNode *p_object);
	NxPs2::CGeomNode *	GetEngineObject() const;

	Mth::Vector			GetBoundingSphere();


private:				// It's all private, as it is machine specific
	virtual CGeom *		plat_clone(bool instance, CScene* pDestScene=NULL);
	virtual CGeom *		plat_clone(bool instance, CModel* pDestModel);

	virtual	bool		plat_load_geom_data(CMesh* pMesh, CModel* pModel, bool color_per_material);
	virtual void		plat_finalize();

	virtual uint32		plat_get_checksum();

	virtual	void		plat_set_color(Image::RGBA rgba);
	virtual	Image::RGBA	plat_get_color() const;
	virtual	void		plat_clear_color();

	virtual bool		plat_set_material_color(uint32 mat_checksum, int pass, Image::RGBA rgba);
	virtual Image::RGBA	plat_get_material_color(uint32 mat_checksum, int pass);

	virtual	void		plat_set_visibility(uint32 mask);
	virtual	uint32		plat_get_visibility() const;

	virtual	void		plat_set_active(bool active);
	virtual	bool		plat_is_active() const;

	virtual const Mth::CBBox &	plat_get_bounding_box() const;

	virtual void	plat_set_bounding_sphere( const Mth::Vector& boundingSphere );
	virtual const Mth::Vector	plat_get_bounding_sphere() const;

	virtual void		plat_set_world_position(const Mth::Vector& pos);
	virtual const Mth::Vector &	plat_get_world_position() const;

	virtual void		plat_set_orientation(const Mth::Matrix& orient);
	virtual const Mth::Matrix &	plat_get_orientation() const;

	virtual void 		plat_rotate_y(Mth::ERot90 rot);

	virtual void		plat_set_transform(const Mth::Matrix& transform);
	virtual const Mth::Matrix &	plat_get_transform() const;

	virtual void		plat_set_scale(const Mth::Vector& scale);
	virtual Mth::Vector plat_get_scale() const;

	virtual void		plat_set_model_lights(CModelLights *);

	virtual bool		plat_render(Mth::Matrix* pRootMatrix, Mth::Matrix* ppBoneMatrices, int numBones);
	virtual bool		plat_hide_polys( uint32 mask );
	virtual bool		plat_enable_shadow( bool enabled );

	virtual	int 		plat_get_num_render_verts();								// - returns number of renderable verts
	virtual	int 		plat_get_num_render_polys();
	virtual	int 		plat_get_num_render_base_polys();
	virtual	void 		plat_get_render_verts(Mth::Vector *p_verts);				// - gets a single array of the verts
	virtual	void 		plat_get_render_colors(Image::RGBA *p_colors);				// - gets an array of vertex colors
	virtual void 		plat_set_render_verts(Mth::Vector *p_verts);				// - sets the verts after modification
	virtual	void 		plat_set_render_colors(Image::RGBA *p_colors);				// - sets the colors after modification

	// Wibble functions
	virtual void		plat_set_uv_wibble_params(float u_vel, float u_amp, float u_freq, float u_phase,
												  float v_vel, float v_amp, float v_freq, float v_phase);
	virtual void		plat_use_explicit_uv_wibble(bool yes);
	virtual void		plat_set_uv_wibble_offsets(float u_offset, float v_offset);
	virtual bool		plat_set_uv_wibble_offsets(uint32 mat_checksum, int pass, float u_offset, float v_offset);
	virtual bool		plat_set_uv_matrix(uint32 mat_checksum, int pass, const Mth::Matrix &mat);

	// local functions
	void 				update_engine_matrix();

private:
	// old format
	NxPs2::CInstance*	mp_oldInstance;
	
	// access to the scene means access to the polys for hiding purposes
	// this code will all be thrown away once we convert to the new geomnode
	// stuff anyway
	CPs2Mesh*			mp_sourceMesh;			

	// new format
	NxPs2::CGeomNode*	mp_instance;
	Mth::Matrix			m_rootMatrix;

	Mth::Matrix *		mp_matrices;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CPs2Geom::SetEngineObject(NxPs2::CGeomNode *p_object)
{
	mp_instance = p_object;
	Dbg_MsgAssert(((int)(mp_instance) & 0xf) == 0,("mp_instance (0x%x) not multiple of 16 (Maybe .SCN corrupted on export?)\n"
													,(int)mp_instance));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline NxPs2::CGeomNode *	CPs2Geom::GetEngineObject() const
{
	return mp_instance;
}

} // Nx

#endif 
