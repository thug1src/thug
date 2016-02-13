//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxGeom.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  3/5/2002
//****************************************************************************

#ifndef	__GFX_P_NX_GEOM_H__
#define	__GFX_P_NX_GEOM_H__
    
#include "gfx/nxgeom.h"
#include "gfx/xbox/p_nxsector.h"
#include "gfx/xbox/p_nxmesh.h"

namespace Mth
{
	class Matrix;
}

namespace NxXbox
{
	class CInstance;
}
						   
namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/////////////////////////////////////////////////////////////////////////////////////
//
// Here's a machine specific implementation of the CGeom
class CXboxGeom : public CGeom
{
                                      
public:
								CXboxGeom();
	virtual 					~CXboxGeom();
	void						SetInstance( NxXbox::CInstance *p_instance )		{ mp_instance = p_instance; }
	NxXbox::CInstance			*GetInstance( void )								{ return mp_instance; }
	void						InitMeshList();
	void						ClearMeshList();
	void						AddMesh( NxXbox::sMesh * );
	Lst::Head< NxXbox::sMesh >	*GetMeshList();

	void						CreateMeshArray();
	bool						RegisterMeshArray( bool just_count );
	void						DestroyMeshArray( void );

	const Mth::CBBox &			GetBoundingBox( void )			{ return m_bbox; }
	void						SetScene( CXboxScene *p_scene )	{ mp_scene = p_scene; }
	NxXbox::sScene				*GenerateScene( void );


private:						// It's all private, as it is machine specific
	virtual	bool					plat_load_geom_data( CMesh *pMesh, CModel *pModel, bool color_per_material );
	virtual void					plat_finalize( void );
	virtual	void					plat_set_active( bool active );
	virtual bool					plat_is_active( void ) const;
	virtual const Mth::CBBox &		plat_get_bounding_box( void ) const;
	virtual const Mth::Vector		plat_get_bounding_sphere( void ) const;
	virtual void					plat_set_bounding_sphere( const Mth::Vector& boundingSphere );

	virtual const Mth::Vector &		plat_get_world_position( void ) const;
	virtual void					plat_set_world_position( const Mth::Vector& pos );
	virtual const Mth::Matrix &		plat_get_orientation( void ) const;
	virtual void					plat_set_orientation( const Mth::Matrix& orient );
	virtual void					plat_set_scale( const Mth::Vector & scale );
	virtual Mth::Vector				plat_get_scale( void ) const;
	virtual void					plat_rotate_y( Mth::ERot90 rot );
	virtual bool					plat_render( Mth::Matrix* pRootMatrix, Mth::Matrix* ppBoneMatrices, int numBones );
	virtual void					plat_set_bone_matrix_data( Mth::Matrix* pBoneMatrices, int numBones );
	virtual bool					plat_hide_polys( uint32 mask );
	virtual uint32					plat_get_visibility( void ) const;
	virtual	void					plat_set_visibility( uint32 mask );
	virtual void					plat_set_color( Image::RGBA rgba );
	virtual void					plat_clear_color( void );
	virtual Image::RGBA				plat_get_color( void ) const;
	virtual	int						plat_get_num_render_verts( void );
	virtual	void					plat_get_render_verts( Mth::Vector *p_verts );
	virtual	void					plat_get_render_colors( Image::RGBA *p_colors );
	virtual void					plat_set_render_verts( Mth::Vector *p_verts );
	virtual	void					plat_set_render_colors( Image::RGBA *p_colors );
	virtual void					plat_set_model_lights( CModelLights* p_model_lights );
	virtual void					plat_set_uv_wibble_params( float u_vel, float u_amp, float u_freq, float u_phase, float v_vel, float v_amp, float v_freq, float v_phase );
	virtual void					plat_use_explicit_uv_wibble( bool yes );
	virtual void					plat_set_uv_wibble_offsets( float u_offset, float v_offset );
	virtual bool					plat_set_uv_wibble_offsets( uint32 mat_name_checksum, int pass, float u_offset, float v_offset );
	virtual bool					plat_set_uv_matrix( uint32 mat_name_checksum, int pass, const Mth::Matrix& mat );
	virtual bool					plat_set_material_color( uint32 mat_name_checksum, int pass, Image::RGBA rgba );
	virtual bool					plat_allocate_uv_matrix_params( uint32 mat_checksum, int pass );

	virtual CXboxGeom*				plat_clone( bool instance, CScene *p_dest_scene = NULL );
	virtual CXboxGeom*				plat_clone( bool instance, CModel *p_dest_model );

public:
	Mth::CBBox						m_bbox;	
	
	CXboxScene *					mp_scene;
	Lst::Head< NxXbox::sMesh >		*mp_init_mesh_list;   
	NxXbox::sMesh **				m_mesh_array;
	uint							m_num_mesh;
	uint32							m_visible;
	bool							m_active;

private:
	NxXbox::CInstance				*mp_instance;
	CXboxMesh						*mp_mesh;		// Used for obtaining CAS poly removal data.
	Mth::Vector						m_scale;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // Nx

#endif 
