//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       NxGeom.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/27/2002
//****************************************************************************

#ifndef	__GFX_NXGEOM_H__
#define	__GFX_NXGEOM_H__
                   
#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#ifndef	__CORE_ROT90_H
    #include <core/math/rot90.h>
#endif

#ifdef __PLAT_NGC__
#include "sys/ngc/p_gx.h"
#endif		// __PLAT_NGC__

namespace Mth
{
	class Matrix;
	class Vector;
	class CBBox;
}
			
namespace Image
{
	struct RGBA;
}

namespace Nx
{
	class CScene;
	class CMesh;
	class CModel;
	class CModelLights;
	class CCollObjTriData;

class CGeom : public Spt::Class
{

public:

    // The basic interface to the geom object
    // this is the machine independent part	
    // machine independent range checking, etc can go here	
						CGeom();
    virtual				~CGeom();

	CGeom *				Clone(bool instance, CScene* pDestScene=NULL);
	CGeom *				Clone(bool instance, CModel* pDestModel);		// Clones a Geom from one model into a new model

	bool				LoadGeomData(CMesh* pMesh, CModel* pModel, bool color_per_material);
	void				Finalize();

	uint32				GetChecksum();

	void				SetColor(Image::RGBA rgba);
	Image::RGBA			GetColor() const;
	void				ClearColor();

	// Can only be used on geometries that were created with color_per_material
	bool				SetMaterialColor(uint32 mat_checksum, int pass, Image::RGBA rgba);
	Image::RGBA			GetMaterialColor(uint32 mat_checksum, int pass);

	void				SetVisibility(uint32 mask);
	uint32				GetVisibility() const;

	void				SetActive(bool active);
	bool				IsActive() const;

	const Mth::CBBox &	GetBoundingBox() const;

	void				SetBoundingSphere(const Mth::Vector& boundingSphere);
	const Mth::Vector	GetBoundingSphere() const;

	void				SetWorldPosition(const Mth::Vector& pos);
	const Mth::Vector &	GetWorldPosition() const;

	void				SetOrientation(const Mth::Matrix& orient);
	const Mth::Matrix &	GetOrientation() const;

	void 				RotateY(Mth::ERot90 rot);

	void				SetTransform(const Mth::Matrix& transform);		// Overrides both world pos and orientation
	const Mth::Matrix &	GetTransform() const;

	void				SetScale(const Mth::Vector& scale);
	Mth::Vector			GetScale() const;

	void				SetCollTriData(CCollObjTriData *);
	CCollObjTriData *	GetCollTriData() const;

	void				SetModelLights(CModelLights *);

	bool                Render( Mth::Matrix* pMatrix, Mth::Matrix* ppBoneMatrices, int numBones );
	void				SetBoneMatrixData( Mth::Matrix* pBoneMatrices, int numBones );
	bool				HidePolys(uint32 mask);
	bool				EnableShadow(bool enabled);

	// used by CModelBuilder to figure out which color modulation function to use
	bool				MultipleColorsEnabled();

// Functions for getting and modifying the raw vertex information in a CGeom	
	int 				GetNumRenderPolys();									// - returns number of renderable polys
	int 				GetNumRenderBasePolys();							// - returns number of first pass polys
	int 				GetNumRenderVerts();								// - returns number of renderable verts
	void 				GetRenderVerts(Mth::Vector *p_verts);				// - gets a single array of the verts
	void 				GetOrigRenderColors(Image::RGBA *p_colors);			// - gets original render colors, storing them if this si the first time
	void 				GetRenderColors(Image::RGBA *p_colors);				// - gets an array of vertex colors
	void 				SetRenderVerts(Mth::Vector *p_verts);				// - sets the verts after modification
	void 				SetRenderColors(Image::RGBA *p_colors);				// - sets the colors after modification

	// Wibble functions
	void				SetUVWibbleParams(float u_vel, float u_amp, float u_freq, float u_phase,
										  float v_vel, float v_amp, float v_freq, float v_phase);
	void				UseExplicitUVWibble(bool yes);
	void				SetUVWibbleOffsets(float u_offset, float v_offset);
	bool				SetUVWibbleOffsets(uint32 mat_checksum, int pass, float u_offset, float v_offset);
	bool				SetUVMatrix(uint32 mat_checksum, int pass, const Mth::Matrix &mat);
	bool				AllocateUVMatrixParams(uint32 mat_checksum, int pass);

protected:
	// Declares if and what type of clone it is
	enum CloneType
	{
		vORIGINAL = 0,			// not cloned
		vINSTANCE,
		vCOPY,
	};

	CloneType			m_cloned;										// marks if cloned object
	CCollObjTriData *	mp_coll_tri_data;								// Collision geom data

	Image::RGBA		*	mp_orig_render_colors;
	bool				m_multipleColorsEnabled;

private:
    // The virtual functions will have a stub implementation
    // in p_nxgeom.cpp
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
	
	virtual void		plat_set_bounding_sphere(const Mth::Vector& boundingSphere);
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
	virtual void		plat_set_bone_matrix_data( Mth::Matrix* pBoneMatrices, int numBones );
	virtual bool		plat_hide_polys(uint32 mask);
	virtual bool		plat_enable_shadow( bool enabled );

	virtual	int 		plat_get_num_render_polys();
	virtual	int 		plat_get_num_render_base_polys();
	virtual	int 		plat_get_num_render_verts();								// - returns number of renderable verts
	virtual	void 		plat_get_render_verts(Mth::Vector *p_verts);				// - gets a single array of the verts
	virtual	void 		plat_get_render_colors(Image::RGBA *p_colors);				// - gets an array of vertex colors
	virtual void 		plat_set_render_verts(Mth::Vector *p_verts);				// - sets the verts after modification
	virtual	void 		plat_set_render_colors(Image::RGBA *p_colors);				// - sets the colors after modification

	virtual void		plat_set_uv_wibble_params(float u_vel, float u_amp, float u_freq, float u_phase,
												  float v_vel, float v_amp, float v_freq, float v_phase);
	virtual void		plat_use_explicit_uv_wibble(bool yes);
	virtual void		plat_set_uv_wibble_offsets(float u_offset, float v_offset);
	virtual bool		plat_set_uv_wibble_offsets(uint32 mat_checksum, int pass, float u_offset, float v_offset);
	virtual bool		plat_set_uv_matrix(uint32 mat_checksum, int pass, const Mth::Matrix &mat);
	virtual bool		plat_allocate_uv_matrix_params(uint32 mat_checksum, int pass);
};

}

#endif // __GFX_NXGEOM_H__

