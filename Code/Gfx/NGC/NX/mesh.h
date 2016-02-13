#ifndef __MESH_H
#define __MESH_H

#include <core\math.h>
#include <core\math\geometry.h>
#include <gfx\nx.h>
#include "material.h"
#include <sys\ngc\p_gx.h>

// Comment in for 16 bit vertices.
//#define SHORT_VERT

namespace NxNgc
{

struct sDLHeader;	// Forward reference.

//#define MESHFLAG_TEXTURE     (1<<0)
//#define MESHFLAG_COLOURS     (1<<1)
//#define MESHFLAG_NORMALS     (1<<2)

//#define OBJFLAG_TEXTURE      (1<<0)
//#define OBJFLAG_COLOURS      (1<<1)
//#define OBJFLAG_NORMALS      (1<<2)
//#define OBJFLAG_TRANSPARENT	 (1<<3)

struct sCASData
{
	uint32	mask;
	uint32	data0;
	uint32	data1;
};
	
// Starting to turn this into a class, but since engine is still
// C-style, I'm keeping everything public.  Garrett
struct sMesh
{
public:
	enum EMeshFlags
	{
		MESH_FLAG_IS_INSTANCE				= (1<<0),
		MESH_FLAG_ACTIVE					= (1<<1),
		MESH_FLAG_IS_CLONED					= (1<<2),
		MESH_FLAG_VISIBLE					= (1<<3),		// Used for batch culling.
		MESH_FLAG_CLONED_POS				= (1<<4),
		MESH_FLAG_CLONED_COL				= (1<<5),
		MESH_FLAG_INDEX_COUNT_SET			= (1<<6),
		MESH_FLAG_NO_SKATER_SHADOW			= (1<<7),
		MESH_FLAG_CLONED_DL					= (1<<8),
	};

					sMesh( void );
					~sMesh( void );

	// Functions
	void			wibble_vc( void );
	void			wibble_normals( void );
	uint32			GetChecksum()	const { return Checksum; }
	uint32			GetFlags()		const			{ return m_flags; }
	
	void			SetActive( bool active )		{ m_flags = ( m_flags & ~MESH_FLAG_ACTIVE ) | ( active ? MESH_FLAG_ACTIVE : 0 ); }
	void			SetPosition( Mth::Vector &pos );
	void			GetPosition( Mth::Vector *p_pos );
	void			SetYRotation( Mth::ERot90 rot );
	void			SetVisibility( uint32 mask )	{ m_visibility_mask	= mask; }
	uint32			GetVisibility( void )			{ return m_visibility_mask; }
	sMesh			*Clone( bool instance = false );
//	void			Initialize( int num_vertices,
//								float* p_positions,
//#ifdef SHORT_VERT
//								s16* p_positions16,
//								int shift,
//								float off_x,
//								float off_y,
//								float off_z,
//#endif		// SHORT_VERT
//								s16* p_normals,
//								float* p_tex_coords,
//								float* p_env_buffer,
//								int num_tc_sets,
//								uint32* p_colors,
//								int num_indices,
//								uint16* p_pos_indices,
//								uint16* p_col_indices,
//								uint16* p_uv_indices,
//								unsigned long material_checksum,
//								void *p_scene,
//								int num_double,
//								uint32* p_double,
//								int num_single,
//								int localMeshIndex,
//								GXPrimitive primitive_type,
//								int bone_idx,
//								char *p_vc_wibble_anims  = NULL );

	void			Build( int num_indices, uint16* p_pos_indices, uint16* p_col_indices, uint16* p_uv_indices, bool rebuild = false );
	void			Rebuild( int num_indices, uint16* p_indices );

	// Note: localMeshIndex is the nth instance of a mesh in a sector.
	void			Submit( void );

	// Members
	uint32					Checksum;
	uint16					m_flags;
	int16					m_bone_idx;
	uint32					m_visibility_mask;
	float					m_bottom_y;	// Use this to cull meshes above the skater for shadows.

//	sMaterial*				mp_material;
//	
//	uint16					m_num_indices;
//	uint16					m_num_vertex;
//	uint8					m_vertex_stride;
//	uint8					m_num_uv_sets;
//
//	uint16					m_display_list_size;
//	uint8					m_primitive_type;
//	uint8					m_vertex_format;
//	uint16					m_localMeshIndex;
//
//	uint16*					mp_index_buffer;
//#ifdef SHORT_VERT
//	s16*					mp_posBuffer;
//#else
//	float*					mp_posBuffer;
//#endif		// SHORT_VERT
//	float					m_offset_x;
//	float					m_offset_y;
//	float					m_offset_z;
//	s16*					mp_normBuffer;
//	float*					mp_uvBuffer;
//	float*					mp_envBuffer;
//	uint32*					mp_colBuffer;
//
//	uint8*					mp_display_list;
//
//	Mth::CBBox				m_bbox;
//	float					m_sphere[4];
//
//	uint16					m_numDouble;
//	uint16					m_numSingle;
//	uint32*					mp_doubleWeight;
//
//	char					*mp_vc_wibble_data;
//
//	GXColor					m_material_color_override;

//	Mth::Vector				m_sphere;

	sDLHeader*				mp_dl;
//	void *					mp_texture_dl;
//	uint32					m_texture_dl_size;
//
//	bool					dl_owner;

	GXColor					m_base_color;
};

/*
// Starting to turn this into a class, but since engine is still
// C-style, I'm keeping everything public.  Garrett
struct sGroup
{
public:
	// Functions
	sMesh*			GetMeshArray(); 
	uint			GetNumMeshes() const { return NumMeshes; }

	// Members
	uint32 ID;
	uint8 *pUpload[2];
	uint8 *pRender[2];
	uint FirstMeshIndex;
	uint NumMeshes;
};


sGroup*			NewMeshGroup(uint32 group_ID, uint32 num_meshes);
sGroup*			LoadMeshGroup(void *p_FH);
void			LoadVertices(void *p_FH, sMesh *pMesh, sMaterial *pMat);

extern sMesh *Meshes;
extern uint NumMeshes, NumMeshGroups;
extern uint NumMeshesInGroup[32];
extern int TotalNumVertices;
*/

void SetMeshScalingParameters( Nx::SMeshScalingParameters* pParams );
void DisableMeshScaling( void );
void ApplyMeshScaling( NxNgc::sObjectHeader * p_object );

} // namespace NxNgc

#endif // __MESH_H

