///////////////////////////////////////////////////////////////////////////////
// p_NxScene.h

#ifndef	__GFX_P_NX_TEXTURE_H__
#define	__GFX_P_NX_TEXTURE_H__

#include 	<gfx/nxtexture.h>
#include 	"gfx/NGPS/NX/texture.h"
#include 	"gfx/NGPS/NX/material.h"
#include 	"gfx/NGPS/NX/scene.h"
#include 	"gfx/NGPS/NX/sprite.h"

namespace Nx
{

// Forward declarations
class CPs2TexDict;

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Machine specific implementation of the CTexture
class	CPs2Texture : public CTexture
{
public:
								CPs2Texture(bool loading_screen = false);
								CPs2Texture(const CPs2Texture & src_texture);		// Copy constructor
	virtual						~CPs2Texture();

	NxPs2::SSingleTexture *		GetSingleTexture() const;
	NxPs2::sTexture *			GetGroupTexture() const;

	static void					sInitTables();

private:		// It's all private, as it is machine specific

	uint8 						find_closest_clut_color(Image::RGBA rgba);

	void						setup_fast_clut_color_find(bool use_alpha = true);
	uint8 						fast_find_closest_clut_color(Image::RGBA rgba);
	void 						cleanup_fast_clut_color_find();

	void						copy_region_to_32bit_buffer(uint32 *p_pixel_buffer, uint16 x_pos, uint16 y_pos,
															uint16 width, uint16 height);
	void						blit_32bit_buffer_to_texture(uint32 *p_pixel_buffer, uint16 x_pos, uint16 y_pos,
															 uint16 width, uint16 height);
	void						set_32bit_pixel_to_texture(uint32 pixel, uint16 x_pos, uint16 y_pos);

	// Plat functions
	virtual bool				plat_load_texture(const char *p_texture_name, bool sprite, bool alloc_vram);
	virtual bool				plat_load_texture_from_buffer(uint8* p_buffer, int buffer_size, bool sprite, bool alloc_vram);
	virtual bool				plat_replace_texture(CTexture *p_texture);

	virtual bool				plat_generate_32bit_image(bool renderble = false, bool store_original = false);
	virtual bool				plat_put_32bit_image_into_texture(bool new_palette = false);

	virtual bool				plat_offset(int x_pixels, int y_pixels, bool use_fill_color, Image::RGBA fill_color);

	virtual bool				plat_adjust_region(uint16 x_pos, uint16 y_pos, uint16 width, uint16 height,
												   int split_axis, uint16 start_point, uint16 end_point);

	virtual bool				plat_pull_to_edge(uint16 point, int axis, int num_pixels);
	virtual bool				plat_push_to_point(uint16 point, int axis, int num_pixels, bool use_fill_color,
												   Image::RGBA fill_color);

	virtual bool				plat_adjust_brightness(float brightness_scale, bool force_adjust_current = false);
	virtual bool				plat_adjust_hsv(float h, float s, float v, bool force_adjust_current = false);

	virtual bool				plat_add_to_vram();
	virtual bool				plat_remove_from_vram();

	virtual uint16				plat_get_width() const;
	virtual uint16				plat_get_height() const;
	virtual uint8				plat_get_bitdepth() const;
	virtual uint8				plat_get_palette_bitdepth() const;
	virtual uint8				plat_get_num_mipmaps() const;
	virtual bool				plat_is_transparent() const;

	virtual bool				plat_combine_textures(CTexture *p_texture, bool palette_gen);

	// Static functions
	static uint8 				s_clut8_index_to_offset(uint8 PS2idx);
	static uint8 				s_clut8_offset_to_index(uint8 offset);

	static inline uint32		s_get_pixel_from_32bit_texture(uint32 *p_buffer, uint16 x_pos, uint16 y_pos, uint16 stride);
	static void					s_get_nearest_pixels_from_32bit_texture(Image::RGBA p_nearest[][2], uint32 *p_buffer,
																		uint16 x_pos, uint16 y_pos, uint16 width, uint16 height);
	
	static void					s_get_region(uint8 *p_buffer, uint16 x_pos, uint16 y_pos, uint16 tex_width, uint16 tex_height,
											 uint8 *p_region, uint16 region_width, uint16 region_height, uint16 bitdepth);
	static void					s_put_region(uint8 *p_buffer, uint16 x_pos, uint16 y_pos, uint16 tex_width, uint16 tex_height,
											 uint8 *p_region, uint16 region_width, uint16 region_height, uint16 bitdepth);

	static void					s_scale_texture(uint8 *p_buffer, uint16 width, uint16 height,
												uint8 *p_result_buffer, uint16 new_width, uint16 new_height, uint16 bitdepth);

	static void					s_combine_adjacent_borders(uint8 *p_first_rect, uint8 *p_second_rect, uint16 first_width,
														   uint16 first_height, uint16 second_stride, float first_portion,
														   int split_axis, uint16 bitdepth);

	uint16						m_width;
	uint16						m_height;
	uint8						m_bitdepth;
	uint8						m_clut_bitdepth;
	uint8						m_num_mipmaps;
	bool						m_transparent;

	// Tells is this texture is used as loading screen (so it isn't allocated VRAM, etc)
	bool						m_loading_screen;

	// The actual data in the engine (can only be one or the other
	NxPs2::SSingleTexture *		mp_single_texture;	// Uses SSingleTexture
	NxPs2::sTexture *			mp_group_texture;	// Uses sTexture (part of texture group

	// Temp 32-bit texture images for texture manipulations (so we don't need to palettize every operation)
	uint32 *					mp_orig_temp_32bit_image;		// Original
	uint32 *					mp_cur_temp_32bit_image;		// Current
	NxPs2::SSingleTexture *		mp_temp_32bit_single_texture;	// So we can draw it

	// PS2 clut index conversions
	static bool					s_tables_initialized;
    static uint8				s_clut8_index_to_offset_table[256];
    static uint8				s_clut8_offset_to_index_table[256];
	
	// Friends
	friend CPs2TexDict;
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline NxPs2::SSingleTexture *	CPs2Texture::GetSingleTexture() const
{
	if (mp_temp_32bit_single_texture)
	{
		return mp_temp_32bit_single_texture;		// This can be a problem if plat_put_32bit_image_into_texture()
													// is called before the corresponding CSprite is deleted.
	}
	else
	{
		return mp_single_texture;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline NxPs2::sTexture *		CPs2Texture::GetGroupTexture() const
{
	return mp_group_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint32					CPs2Texture::s_get_pixel_from_32bit_texture(uint32 *p_buffer, uint16 x_pos, uint16 y_pos,
																			uint16 stride)
{
	return p_buffer[(y_pos * stride) + x_pos];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint8 					CPs2Texture::s_clut8_index_to_offset(uint8 PS2idx)
{
    return s_clut8_index_to_offset_table[PS2idx];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint8 					CPs2Texture::s_clut8_offset_to_index(uint8 offset)
{
    return s_clut8_offset_to_index_table[offset];
}

//////////////////////////////////////////////////////////////////////////////////
// Machine specific implementation of the CMaterial
class	CPs2Material : public CMaterial
{
public:
								CPs2Material();
	virtual						~CPs2Material();

private:
	virtual Image::RGBA			plat_get_rgba() const;
	virtual void				plat_set_rgba(Image::RGBA rgba);
	virtual void				plat_set_texture();

	Image::RGBA					m_rgba;

	// The actual data in the engine
	NxPs2::sMaterial *			mp_material;
};

//////////////////////////////////////////////////////////////////////////////////
// Machine specific implementation of the CTexDict
class	CPs2TexDict : public CTexDict
{
public:
								CPs2TexDict(uint32 checksum);		// loads nothing
								CPs2TexDict(uint32* pData, int dataSize, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup = false);
								CPs2TexDict(const char *p_tex_dict_name, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup = false);
	virtual						~CPs2TexDict();

	NxPs2::sScene *				GetEngineTextureDictionary() const;

public:
	// made this public so that the tex dict manager needs to be able to 
	// create a texture dictionary and load up its textures in two separate steps...
	bool						LoadTextureDictionary(const char *p_tex_dict_name, uint32* pData, int dataSize, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup = false);

private:
	void						add_textures_to_hash_table();
	bool						UnloadTextureDictionary();

	// Platform-specific calls
	virtual CTexture *			plat_load_texture(const char *p_texture_name, bool sprite, bool alloc_vram);
	virtual CTexture *			plat_load_texture_from_buffer(uint8* p_buffer, int buffer_size, uint32 texture_checksum, bool sprite, bool alloc_vram);
	virtual CTexture *			plat_reload_texture(const char *p_texture_name);
	virtual bool				plat_unload_texture(CTexture *p_texture);
	virtual void				plat_add_texture(CTexture *p_texture);
	virtual bool				plat_remove_texture(CTexture *p_texture);
	virtual CTexture *			plat_copy_texture(uint32 new_texture_checksum, CTexture *p_texture);
	//virtual CTexture *			plat_combine_textures(uint32 new_texture_checksum, CTexture *p_texture1, CTexture *p_texture2);

	NxPs2::sScene *				mp_tex_dict;		// Platform-dependent data
};

} // Namespace Nx  			

#endif
