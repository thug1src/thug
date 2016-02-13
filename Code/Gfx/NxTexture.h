///////////////////////////////////////////////////////////////////////////////////
// NxTexture.H - Neversoft Engine, Rendering portion, Platform independent interface

#ifndef	__GFX_NX_TEXTURE_H__
#define	__GFX_NX_TEXTURE_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/hashtable.h>

#include <gfx/image/imagebasic.h>


namespace Nx
{

class CTexDict;
class CTexDictManager;

//////////////////////////////////////////////////////////////////////////////////
// Nx::CTexture replaces RwTexture
class	CTexture
{
public:
							CTexture();
							CTexture(const CTexture & src_texture);		// Copy constructor
	virtual					~CTexture();

	bool					LoadTexture(const char *p_texture_name, bool sprite, bool alloc_vram = true);
	bool					LoadTextureFromBuffer(uint8* p_buffer, int buffer_size, uint32 texture_checksum, bool sprite, bool alloc_vram = true);
	bool					ReplaceTexture(CTexture *p_texture);

	// Temp 32-bit texture image generation.  All the texture manipulation routines need to use
	// 32-bit versions of the texture.  But to palettize every operation can be slow, so keeping
	// the temp buffer around speeds things up greatly.
	bool					Generate32BitImage(bool renderble = false, bool store_original = false);
	bool					Put32BitImageIntoTexture(bool new_palette = false);

	// Combines current texture with new texture (overwriting original one)
	bool					CombineTextures(CTexture *p_texture, bool palette_gen = true);

	// Move texture (cropping the part that moves off and filling in the new parts with either the old edge or a
	// supplied fill color)
	bool					Offset(int x_pixels, int y_pixels, bool use_fill_color = false,
								   Image::RGBA fill_color = Image::RGBA(128, 128, 128, 128));

	// This function will scale a rectangular section of a texture.  Each side of the texture
	// will be stretched or shrunk from the start_point to the end_point along the split axis.
	bool					AdjustRegion(uint16 x_pos, uint16 y_pos, uint16 width, uint16 height,
										 int split_axis, uint16 start_point, uint16 end_point);

	// Pull texture from point along the axis by num_pixels (+ or - determines the direction)
	// and crop anything going outside the texture border.
	bool					PullToEdge(uint16 point, int axis, int num_pixels);
	// Similar to PullToEdge(), except it pushes the texture from the edge to the point.  Instead
	// of cropping, it either stretches the edge line or fills it in with a fill color
	bool					PushToPoint(uint16 point, int axis, int num_pixels, bool use_fill_color = false,
										Image::RGBA fill_color = Image::RGBA(128, 128, 128, 128));

	bool					AdjustBrightness(float brightness_scale, bool force_adjust_current = false);
	bool					AdjustHSV(float h, float s, float v, bool force_adjust_current = false);

	// These VRAM calls won't do anything on platforms that don't have separate
	// texture memory
	bool					AddToVram();
	bool					RemoveFromVram();

	uint32					GetChecksum() const;
	uint16					GetWidth() const;
	uint16					GetHeight() const;
	uint8					GetBitDepth() const;
	uint8					GetPaletteBitDepth() const;
	uint8					GetNumMipmaps() const;
	bool					IsTransparent() const;

	void					SetPerm( bool perm) {m_perm = perm;}
	bool					IsPerm() {return m_perm;}

#ifdef	__NOPT_ASSERT__
	char *					GetName() {return mp_name;}
#else
	char *					GetName() {return "WARNING NO NAME";}
#endif	
	

protected:

	uint32					m_checksum;

	// So it can access set_checksum()
	friend CTexDict;

private:
	virtual bool			plat_load_texture(const char *p_texture_name, bool sprite, bool alloc_vram);
	virtual bool			plat_load_texture_from_buffer(uint8* p_buffer, int buffer_size, bool sprite, bool alloc_vram);
	virtual bool			plat_replace_texture(CTexture *p_texture);

	virtual bool			plat_generate_32bit_image(bool renderble = false, bool store_original = false);
	virtual bool			plat_put_32bit_image_into_texture(bool new_palette = false);

	virtual bool			plat_offset(int x_pixels, int y_pixels, bool use_fill_color, Image::RGBA fill_color);

	virtual bool			plat_adjust_region(uint16 x_pos, uint16 y_pos, uint16 width, uint16 height,
											   int split_axis, uint16 start_point, uint16 end_point);

	virtual bool			plat_pull_to_edge(uint16 point, int axis, int num_pixels);
	virtual bool			plat_push_to_point(uint16 point, int axis, int num_pixels, bool use_fill_color,
											   Image::RGBA fill_color);

	virtual bool			plat_adjust_brightness(float brightness_scale, bool force_adjust_current = false);
	virtual bool			plat_adjust_hsv(float h, float s, float v, bool force_adjust_current = false);

	virtual bool			plat_add_to_vram();
	virtual bool			plat_remove_from_vram();

	virtual uint16			plat_get_width() const;
	virtual uint16			plat_get_height() const;
	virtual uint8			plat_get_bitdepth() const;
	virtual uint8			plat_get_palette_bitdepth() const;
	virtual uint8			plat_get_num_mipmaps() const;
	virtual bool			plat_is_transparent() const;

	virtual bool			plat_combine_textures(CTexture *p_texture, bool palette_gen);
	
	bool					m_perm;		// set to true if this texture should survive a level change 
	
	#ifdef	__NOPT_ASSERT__
	char	*				mp_name;
	#endif

};

//////////////////////////////////////////////////////////////////////////////////
// Nx::CMaterial
class	CMaterial
{
public:
							CMaterial();
	virtual					~CMaterial()	{}

	Image::RGBA				GetRGBA() const;
	void					SetRGBA(Image::RGBA rgba);
	CTexture *				GetTexture() const;
	void					SetTexture(CTexture *tex);

protected:
	uint32					m_checksum;
	CTexture *				mp_texture;

private:
	virtual Image::RGBA		plat_get_rgba() const;
	virtual void			plat_set_rgba(Image::RGBA rgba);
	virtual void			plat_set_texture(CTexture *tex);

};

//////////////////////////////////////////////////////////////////////////////////
// Nx::CTexDict
class	CTexDict
{
public:
	// The constructor and destructor start the loading and unloading
							CTexDict(uint32 checksum, bool create_lookup_table = true);	// loads nothing
							CTexDict(const char *p_tex_dict_name, bool create_lookup_table = false);
	virtual					~CTexDict();

	void					IncRefCount();
	int16					DecRefCount();

	// The load and unload functions probably will change, since
	// we will only load and unload whole texture dictionaries.
	// For now, they will only be used for the 2D textures.
	CTexture *				LoadTexture(const char *p_texture_name, bool sprite, bool alloc_vram = true, bool perm = true);
	CTexture *				LoadTextureFromBuffer(uint8* p_buffer, int buffer_size, uint32 texture_checksum, bool sprite, bool alloc_vram = true, bool perm = true);
	CTexture *				ReloadTexture(const char *p_texture_name);
	bool					UnloadTexture(CTexture *p_texture);
	void					AddTexture(CTexture *p_texture);
	bool					RemoveTexture(CTexture *p_texture);
	bool					ReplaceTexture( const char* p_src_texture_name, const char* p_dst_texture_name );
	bool					ReplaceTexture( const char* p_src_texture_name, CTexture* p_dst_texture );

	CTexture *				CopyTexture(uint32 new_texture_checksum, CTexture *p_texture);
	// Combine two textures to make a new texture
	CTexture *				CombineTextures(uint32 new_texture_checksum, CTexture *p_texture1, CTexture *p_texture2, bool palette_gen = true);

	uint32					GetChecksum() const;
	CTexture *  			GetTexture(uint32 checksum) const;
	CTexture *  			GetTexture(const char *p_texture_name) const;
	
	uint32					GetFileSize() {return m_file_size;}

	Lst::HashTable<CTexture> *GetTexLookup( void ) { return mp_texture_lookup; }

	void					FlushTextures(bool flush_all);	// flush textures, optionally flushing perm textures

protected:
	void					set_checksum(uint32 checksum);
	CTexture*				get_source_texture( const char* p_src_texture_name );

	uint32					m_checksum;
	int16					m_ref_count;

	Lst::HashTable<CTexture> *	mp_texture_lookup;

	// So it can access set_checksum()
	friend CTexDictManager;

private:	
	// Platform-specific calls
	virtual CTexture *			plat_load_texture(const char *p_texture_name, bool sprite, bool alloc_vram);
	virtual CTexture *			plat_load_texture_from_buffer(uint8* p_buffer, int buffer_size, uint32 texture_checksum, bool sprite, bool alloc_vram);
	virtual CTexture *			plat_reload_texture(const char *p_texture_name);
	virtual bool				plat_unload_texture(CTexture *p_texture);
	virtual void				plat_add_texture(CTexture *p_texture);
	virtual bool				plat_remove_texture(CTexture *p_texture);
	virtual CTexture *			plat_copy_texture(uint32 new_texture_checksum, CTexture *p_texture);

protected:
	uint32						m_file_size;		// DEBUGGING USE ONLY, NOT GUARENTEED							 
								 
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void			CTexDict::IncRefCount()
{
	m_ref_count++;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline int16		CTexDict::DecRefCount()
{
	return --m_ref_count;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint32		CTexDict::GetChecksum() const
{
	return m_checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void			CTexDict::set_checksum(uint32 checksum)
{
	m_checksum = checksum;
}

}


#endif

