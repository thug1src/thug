///////////////////////////////////////////////////////////////////////////////
// p_NxScene.h

#ifndef	__GFX_P_NX_TEXTURE_H__
#define	__GFX_P_NX_TEXTURE_H__

#include 	"Gfx/NxTexture.h"
#include 	"Gfx/Ngc/nx/texture.h"
#include 	"Gfx/Ngc/nx/material.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Machine specific implementation of the CTexture
class	CNgcTexture : public CTexture
{
public:
								CNgcTexture();
	virtual						~CNgcTexture();

	NxNgc::sTexture			*GetEngineTexture() const;
	void						SetEngineTexture( NxNgc::sTexture *p_texture )	{ mp_texture = p_texture; }


private:		// It's all private, as it is machine specific
	virtual bool				plat_load_texture( const char *p_texture_name, bool sprite, bool alloc_vram );
	virtual bool				plat_replace_texture( CTexture *p_texture );
	virtual bool				plat_add_to_vram( void );
	virtual bool				plat_remove_from_vram( void );

	virtual uint16				plat_get_width() const;
	virtual uint16				plat_get_height() const;
	virtual uint8				plat_get_bitdepth() const;
	virtual uint8				plat_get_num_mipmaps() const;
	virtual bool				plat_is_transparent() const;

	// The actual data in the engine
	NxNgc::sTexture *			mp_texture;

};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline NxNgc::sTexture	*CNgcTexture::GetEngineTexture() const
{
	return mp_texture;
}



//////////////////////////////////////////////////////////////////////////////////
// Machine specific implementation of the CMaterial
class	CNgcMaterial : public CMaterial
{
public:
								CNgcMaterial();
	virtual						~CNgcMaterial();

private:
	Image::RGBA					plat_get_rgba() const;
	void						plat_set_rgba(Image::RGBA rgba);
	void						plat_set_texture();

	Image::RGBA					m_rgba;

	// The actual data in the engine
	NxNgc::sMaterial *			mp_material;
};

//////////////////////////////////////////////////////////////////////////////////
// Machine specific implementation of the CTexDict
class	CNgcTexDict : public CTexDict
{
public:
								CNgcTexDict( uint32 checksum );		// loads nothing
								CNgcTexDict(const char *p_tex_dict_name, bool forceTexDictLookup = false);
	virtual						~CNgcTexDict();

	bool						LoadTextureDictionary(const char *p_tex_dict_name, bool forceTexDictLookup = false);
	bool						LoadTextureDictionaryFromMemory( void *p_mem );
	bool						UnloadTextureDictionary();

private:
	// Platform-specific calls
	virtual CTexture *			plat_load_texture( const char *p_texture_name, bool sprite, bool alloc_vram );
	virtual CTexture *			plat_reload_texture(const char *p_texture_name);
	virtual bool				plat_unload_texture(CTexture *p_texture);
	virtual void				plat_add_texture(CTexture *p_texture);
	virtual bool				plat_remove_texture(CTexture *p_texture);

//	NxNgc::sScene *			mp_tex_dict;		// Platform-dependent data

};

void LoadTextureFile( const char *Filename, Lst::HashTable<Nx::CTexture> *p_texture_table );
void LoadTextureFileFromMemory( void **pp_mem, Lst::HashTable<Nx::CTexture> *p_texture_table );

} // Namespace Nx  			

#endif

