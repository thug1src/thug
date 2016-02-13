#ifndef __SCENE_H
#define __SCENE_H


#include <core/defines.h>
#include <core/math.h>
#include <core/math/geometry.h>
#include "texture.h"
#include "mesh.h"
#include "material.h"
#include "anim.h"
#include <sys/ngc/p_vector.h>
#include <sys/ngc/p_gx.h>
#include <gfx/nxhierarchy.h>

namespace NxNgc
{

struct sMeshEntry
{
	sMesh*					mp_mesh;			// Pointer to mesh.
	int						m_bbox;				// Bounding box index.
};

#define SCENE_FLAG_RENDERING_SHADOW		( 1 << 7 )
#define SCENE_FLAG_RECEIVE_SHADOWS		( 1 << 8 )
#define SCENE_FLAG_SELF_SHADOWS			( 1 << 9 )
#define SCENE_FLAG_CLONED_GEOM			( 1 << 10 )

struct sScene
{
								sScene( void );
								~sScene( void );

	sMaterial *					sScene::GetMaterial( uint32 checksum );
	void						AddMeshes( int num_meshes, sMesh **pp_meshes );
	void						CountMeshes( int num_meshes, sMesh **pp_meshes );
	void						CreateMeshArrays( void );
	void						RemoveMeshes( int num_meshes, sMesh **pp_meshes );
	void						SortMeshes( void );
	void						FigureBoundingVolumes( void );
	void						HidePolys( uint32 mask, sCASData *p_cas_data, uint32 num_entries );

//	uint32						m_flags;
//	int							NumTextures;
//	uint8						*pTexBuffer;
//	uint8						*pTexDma;
//	sTexture					*pTextures;
//
//	int							m_num_materials;
//	sMaterial *					mp_material_array;
//	
//	// New style, with separate opaque and semitransparent mesh lists.
//	sMesh						**m_opaque_meshes;
//	int							m_num_opaque_entries;
//	int							m_num_filled_opaque_entries;
//	sMesh						**m_semitransparent_meshes;
//	int							m_num_semitransparent_entries;
//	int							m_num_filled_semitransparent_entries;
//	int							m_first_dynamic_sort_entry;
//	int							m_num_dynamic_sort_entries;
////	int							m_num_bboxes;
//	
////	class CInstance				*pInstances;
////
////	sScene						*pNext;
////
////	static sScene				*pHead;
//
//	
	bool						m_is_dictionary;
//
//	Mth::CBBox					m_bbox;	


	// New stuff.
	uint32						m_flags;

	sSceneHeader *				mp_scene_data;		// This is all data for pointers below.
	float *						mp_pos_pool;
	s16 *						mp_nrm_pool;
	uint32 *					mp_col_pool;
	s16 *						mp_tex_pool;
	sDLHeader *					mp_dl;
	sMaterialDL *				mp_blend_dl;
	sTextureDL *				mp_texture_dl;
	sMaterialVCWibbleKeyHeader *mp_vc_wibble;
	sMaterialHeader *			mp_material_header;
	sMaterialUVWibble *			mp_uv_wibble;
	sMaterialPassHeader *		mp_material_pass;
	uint16 *					mp_shadow_volume_mesh;
	sShadowEdge *				mp_shadow_edge;

	Mth::Vector					m_sphere;

	sMesh						**mpp_mesh_list;
	uint16						m_num_meshes;
	uint16						m_num_filled_meshes;

	uint16						m_num_opaque_meshes;
	uint16						m_num_pre_semitrans_meshes;
	uint16						m_num_dynamic_semitrans_meshes;
	uint16						m_num_post_semitrans_meshes;

//	uint16						m_num_opaque_entries;
//	uint16						m_num_semitrans_entries;

	// For mesh heirarchies
	Nx::CHierarchyObject*		mp_hierarchyObjects;						// array of hierarchy objects
	int							m_numHierarchyObjects;						// number of hierarchy objects
};


sScene	*LoadScene( const char *Filename, sScene *pScene );
void	DeleteScene( sScene *pScene );

#define MATERIAL_GROUP_SU	(1<<0)
#define MATERIAL_GROUP_CP	(1<<1)
#define MATERIAL_GROUP_A	(1<<2)
#define MATERIAL_GROUP_B	(1<<3)
#define MATERIAL_GROUP_C	(1<<4)

void ResetMaterialChange( void );
void MaterialSubmit( sMesh * p_mesh, sScene *pScene = NULL );

void MaterialBuild(  sMesh * p_mesh, sScene * p_scene, bool bl, bool tx ); 

} // namespace NxNgc


#endif // __SCENE_H


