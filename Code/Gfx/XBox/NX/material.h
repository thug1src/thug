#ifndef __MATERIAL_H
#define __MATERIAL_H

#include <core/HashTable.h>
#include <gfx/image/imagebasic.h>
#include <gfx/nxtexture.h>
#include "texture.h"

namespace NxXbox
{

// Material Flags
#define MATFLAG_UV_WIBBLE					(1<<0)
#define MATFLAG_VC_WIBBLE					(1<<1)
#define MATFLAG_TEXTURED					(1<<2)
#define MATFLAG_ENVIRONMENT					(1<<3)
#define MATFLAG_DECAL						(1<<4)
#define MATFLAG_SMOOTH						(1<<5)
#define MATFLAG_TRANSPARENT					(1<<6)
#define MATFLAG_PASS_COLOR_LOCKED			(1<<7)
#define MATFLAG_SPECULAR					(1<<8)		// Specular lighting is enabled on this material (Pass0).
#define MATFLAG_BUMP_SIGNED_TEXTURE			(1<<9)		// This pass uses an offset texture which needs to be treated as signed data.
#define MATFLAG_BUMP_LOAD_MATRIX			(1<<10)		// This pass requires the bump mapping matrix elements to be set up.
#define MATFLAG_PASS_TEXTURE_ANIMATES		(1<<11)		// This pass has a texture which animates.
#define MATFLAG_PASS_IGNORE_VERTEX_ALPHA	(1<<12)		// This pass should not have the texel alpha modulated by the vertex alpha.
#define MATFLAG_EXPLICIT_UV_WIBBLE			(1<<14)		// Uses explicit uv wibble (set via script) rather than calculated.
#define MATFLAG_WATER_EFFECT				(1<<27)		// This material should be processed to provide the water effect.
#define MATFLAG_NO_MAT_COL_MOD				(1<<28)		// No material color modulation required (all passes have m.rgb = 0.5).


const uint32 MAX_PASSES = 4;


	
struct sUVWibbleParams
{
			sUVWibbleParams( void );
			~sUVWibbleParams( void );

	float	m_UVel;
	float	m_VVel;
	float	m_UFrequency;
	float	m_VFrequency;
	float	m_UAmplitude;
	float	m_VAmplitude;
	float	m_UPhase;
	float	m_VPhase;
	float	m_UVMatrix[4];		// This value is written to dynamically. The first two values are rotation, the second two are translation.
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


struct sTextureWibbleKeyframe
{
	int			m_time;
	sTexture	*mp_texture;
};

struct sTextureWibbleParams
{
	uint32					m_num_keyframes[MAX_PASSES];
	int						m_phase[MAX_PASSES];
	int						m_num_iterations[MAX_PASSES];
	sTextureWibbleKeyframe	*mp_keyframes[MAX_PASSES];
};



struct sMaterial
{
	public:

	static const uint32		BLEND_MODE_MASK	= 0x00FFFFFFUL;

							sMaterial( void );
							~sMaterial( void );
	
	void					Submit( void );
	uint32					GetIgnoreVertexAlphaPasses( void );
	void					figure_wibble_uv( void );
	void					figure_wibble_vc( void );
	void					figure_wibble_texture( void );

	uint32					m_checksum;
	uint32					m_name_checksum;
	uint32					m_passes;

	bool					m_sorted;
	bool					m_no_bfc;
	bool					m_uv_wibble;
	bool					m_texture_wibble;
	uint8					m_alpha_cutoff;
	uint8					m_zbias;

	float					m_grass_height;
	int						m_grass_layers;
	float					m_draw_order;
	uint32					m_flags[MAX_PASSES];
	sTexture*				mp_tex[MAX_PASSES];
	float					m_color[MAX_PASSES][4];				// Element [pass][3] holds the fixed alpha value where appropriate.
	uint32					m_reg_alpha[MAX_PASSES];			// Low 24 bits are blend mode, high 8 bits are fixed alpha value.
	uint32					m_uv_addressing[MAX_PASSES];
	float					m_envmap_tiling[MAX_PASSES][2];		// Tile multiples for env mapping (NOTE: could maybe be changed to byte array?)
	uint32					m_filtering_mode[MAX_PASSES];
	sUVWibbleParams			*mp_UVWibbleParams[MAX_PASSES];
	float					m_k[MAX_PASSES];
	uint32					m_num_wibble_vc_anims;
	sVCWibbleParams			*mp_wibble_vc_params;
	D3DCOLOR				*mp_wibble_vc_colors;				// Max of eight banks of vertex color wibble information.
	sTextureWibbleParams	*mp_wibble_texture_params;
	float					m_specular_color[4];				// Specular color (0-2) plus power term (3).
};


Lst::HashTable< sMaterial >	*LoadMaterials( void *p_FH, Lst::HashTable< Nx::CTexture > *p_texture_table );
Lst::HashTable< sMaterial >	*LoadMaterialsFromMemory( void **pp_mem, Lst::HashTable< Nx::CTexture > *p_texture_table );

//extern Lst::HashTable< sMaterial > *pMaterialTable;
extern uint32 NumMaterials;

} // namespace NxXbox

#endif // __MATERIAL_H

