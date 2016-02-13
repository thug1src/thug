///////////////////////////////////////////////////////////////////////////////
// p_NxScene.h

#ifndef	__GFX_P_NX_TEXTURE_H__
#define	__GFX_P_NX_TEXTURE_H__

#include 	"Gfx/NxTexture.h"
#include 	"Gfx/xbox/nx/texture.h"
#include 	"Gfx/xbox/nx/material.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Machine specific implementation of the CTexture
class	CXboxTexture : public CTexture
{
public:
								CXboxTexture();
	virtual						~CXboxTexture();

	NxXbox::sTexture			*GetEngineTexture() const;
	void						SetEngineTexture( NxXbox::sTexture *p_texture );


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

	uint8						m_num_mipmaps;
	bool						m_transparent;

	// The actual data in the engine
	NxXbox::sTexture *			mp_texture;

};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline NxXbox::sTexture	*CXboxTexture::GetEngineTexture() const
{
	return mp_texture;
}



//////////////////////////////////////////////////////////////////////////////////
// Machine specific implementation of the CMaterial
class	CXboxMaterial : public CMaterial
{
public:
								CXboxMaterial();
	virtual						~CXboxMaterial();

private:
	Image::RGBA					plat_get_rgba() const;
	void						plat_set_rgba(Image::RGBA rgba);
	void						plat_set_texture();

	Image::RGBA					m_rgba;

	// The actual data in the engine
	NxXbox::sMaterial *			mp_material;
};

//////////////////////////////////////////////////////////////////////////////////
// Machine specific implementation of the CTexDict
class	CXboxTexDict : public CTexDict
{
public:
								CXboxTexDict( uint32 checksum );		// loads nothing
								CXboxTexDict(const char *p_tex_dict_name);
	virtual						~CXboxTexDict();

	bool						LoadTextureDictionary( const char *p_tex_dict_name );
	bool						LoadTextureDictionaryFromMemory( void *p_mem );
	bool						UnloadTextureDictionary();

private:

	// Platform-specific calls
	virtual CTexture *			plat_load_texture( const char *p_texture_name, bool sprite, bool alloc_vram );
	virtual CTexture *			plat_reload_texture( const char *p_texture_name );
	virtual bool				plat_unload_texture( CTexture *p_texture );
	virtual void				plat_add_texture( CTexture *p_texture );
	virtual bool				plat_remove_texture( CTexture *p_texture );
};

Lst::HashTable<Nx::CTexture>*	LoadTextureFile( const char *Filename, Lst::HashTable<Nx::CTexture> *p_texture_table, bool okay_to_rebuild_texture_table = false );
Lst::HashTable<Nx::CTexture>*	LoadTextureFileFromMemory( void **pp_mem, Lst::HashTable<Nx::CTexture> *p_texture_table, bool okay_to_rebuild_texture_table = false );


} // Namespace Nx  			

#endif
