#ifndef __SCENE_H
#define __SCENE_H


#include <core/defines.h>
#include <core/math.h>
#include <core/math/geometry.h>
#include <gfx/NxHierarchy.h>
#include "texture.h"
#include "mesh.h"
#include "material.h"
#include "anim.h"

namespace NxXbox
{


struct sMeshEntry
{
	sMesh*					mp_mesh;			// Pointer to mesh.
	int						m_bbox;				// Bounding box index.
};

#define SCENE_FLAG_RENDERING_SHADOW		( 1 << 7 )
#define SCENE_FLAG_RECEIVE_SHADOWS		( 1 << 8 )
#define SCENE_FLAG_SELF_SHADOWS			( 1 << 9 )

struct sScene
{
								sScene( void );
								~sScene( void );

	sMaterial *					sScene::GetMaterial( uint32 checksum );
	void						CountMeshes( int num_meshes, sMesh **pp_meshes );
	void						CreateMeshArrays( void );
	void						AddMeshes( int num_meshes, sMesh **pp_meshes );
	void						RemoveMeshes( int num_meshes, sMesh **pp_meshes );
	void						SortMeshes( void );
	sMesh						*GetMeshByLoadOrder( int load_order );
	void						FigureBoundingVolumes( void );
	void						HidePolys( uint32 mask, sCASData *p_cas_data, uint32 num_entries );
	
	uint32						m_flags;
	int							NumTextures;
	uint8						*pTexBuffer;
	uint8						*pTexDma;
	sTexture					*pTextures;

	int							NumMaterials;
	Lst::HashTable< sMaterial >	*pMaterialTable;
	
	sMesh						**m_meshes;
	int							m_num_mesh_entries;
	int							m_num_semitransparent_mesh_entries;		// Used for making scene level draw order decisions.
	int							m_num_filled_mesh_entries;
	int							m_first_semitransparent_entry;
	int							m_first_dynamic_sort_entry;
	int							m_num_dynamic_sort_entries;
	
	class CInstance				*pInstances;

	sScene						*pNext;

	static sScene				*pHead;

	bool						m_is_dictionary;

	Mth::CBBox					m_bbox;	
	D3DXVECTOR3					m_sphere_center;
	float						m_sphere_radius;

	// For mesh heirarchies.
	Nx::CHierarchyObject*		mp_hierarchyObjects;						// Array of hierarchy objects.
	int							m_numHierarchyObjects;						// Number of hierarchy objects.
};


sScene	*LoadScene( const char *Filename, sScene *pScene );
void	DeleteScene( sScene *pScene );
int		sort_by_material_draw_order( const void *p1, const void *p2 );


} // namespace NxXbox


#endif // __SCENE_H

