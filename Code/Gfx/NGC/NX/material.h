#ifndef __MATERIAL_H
#define __MATERIAL_H

#include <core/HashTable.h>
#include "texture.h"
#include <gfx/nxtexture.h>

//#define __USE_MAT_DL__		// Comment out to save memory.

namespace NxNgc
{

// Material Flags
#define MATFLAG_UV_WIBBLE   (1<<0)
#define MATFLAG_VC_WIBBLE   (1<<1)
#define MATFLAG_TEXTURED    (1<<2)
#define MATFLAG_ENVIRONMENT (1<<3)
#define MATFLAG_DECAL       (1<<4)
#define MATFLAG_SMOOTH      (1<<5)
#define MATFLAG_TRANSPARENT (1<<6)


const uint32 MAX_PASSES = 4;

	
struct sUVWibbleParams
{
	float	m_UVel;
	float	m_VVel;
	float	m_UFrequency;
	float	m_VFrequency;
	float	m_UAmplitude;
	float	m_VAmplitude;
	float	m_UPhase;
	float	m_VPhase;
};

struct sUVControl
{
	sUVWibbleParams	UVWibbleParams[MAX_PASSES];         // 128
	float			UVWibbleOffset[MAX_PASSES][2];		// 32
};

struct sVCWibbleKeyframe
{
	int			m_time;
	Image::RGBA	m_color;
};



struct sVCWibbleParams
{
	uint32				m_num_keyframes;
	int					m_phase;
	sVCWibbleKeyframe	*mp_keyframes;
};




struct sMaterial
{
//	sMaterial( void );
//	~sMaterial( void );

	void			Submit( uint32 mesh_flags, GXColor mesh_color );
	void			Initialize( uint32 mesh_flags, GXColor mesh_color );
	void			figure_wibble_uv( void );
	void			figure_wibble_vc( void );
		
	uint32			Checksum;							// 4
	uint8			Passes;                             // 1
	uint8			m_sorted;                           // 1
	uint8			m_no_bfc;                           // 1
	uint8			AlphaCutoff;                        // 1
	float			m_draw_order;                       // 4
	uint32			Flags[MAX_PASSES];                  // 16
	sTexture*		pTex[MAX_PASSES];                   // 16
	uint8			blendMode[MAX_PASSES];              // 4
	uint8			fixAlpha[MAX_PASSES];               // 4
	uint8			UVAddressing[MAX_PASSES];           // 4
	float			K[MAX_PASSES];						// 16

	char			uv_slot[MAX_PASSES];				// 4
	GXColor			matcol[4];							// 16

#ifdef __USE_MAT_DL__
	uint8*			mp_display_list;					// 4
	uint16			m_display_list_size;				// 2
#endif		// __USE_MAT_DL__
	uint8			m_wibble_uv_update;					// 1
	uint8			m_num_wibble_vc_anims;				// 1
	uint16			pad;
//	uint8			m_grassify;
//	uint8			m_grass_layers;
//	float			m_grass_height;

	sUVControl*		m_pUVControl;						// 4

	float			m_wibble_uv_time;					// 4
	sVCWibbleParams	*mp_wibble_vc_params;				// 4
	uint32			*mp_wibble_vc_colors;				// 4 (116) Max of eight banks of vertex color wibble infortation.
};


sMaterial * LoadMaterials( void *p_FH, Lst::HashTable< Nx::CTexture > *p_texture_table, int * p_num_materials );
sMaterial * LoadMaterialsFromMemory( void **pp_mem, Lst::HashTable< Nx::CTexture > *p_texture_table, int * p_num_materials );
void InitializeMaterial( sMaterial* p_material );

//extern Lst::HashTable< sMaterial > *pMaterialTable;
extern uint32 NumMaterials;

} // namespace NxNgc

#endif // __MATERIAL_H


