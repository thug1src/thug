///////////////////////////////////////////////////////////////////////////////
// p_NxTexture.cpp

#include 	"Gfx/Nx.h"
#include 	"Gfx/xbox/p_NxTexture.h"
#include	"sys/file/filesys.h"

namespace Nx
{

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CTexture

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxTexture::CXboxTexture() :  m_transparent( false ), mp_texture( NULL )
{
	m_num_mipmaps = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxTexture::~CXboxTexture()
{
	if( mp_texture )
	{
		delete mp_texture;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxTexture::SetEngineTexture( NxXbox::sTexture *p_texture )
{
	mp_texture	= p_texture;
	m_checksum	= p_texture->Checksum;
}




/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxTexture::plat_load_texture( const char *p_texture_name, bool sprite, bool alloc_vram )
{
	char filename[256];

	strcpy( filename, p_texture_name );
	
	// Append '.img.xbx' to the end.
	strcat( filename, ".img.xbx" );

	mp_texture = NxXbox::LoadTexture( filename );
	
	return mp_texture;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxTexture::plat_replace_texture( CTexture *p_texture )
{								 	
	CXboxTexture *p_xbox_texture = static_cast<CXboxTexture *>( p_texture );

	// Go through and copy the texture.
	NxXbox::sTexture *p_src	= p_xbox_texture->GetEngineTexture();
	NxXbox::sTexture *p_dst	= GetEngineTexture();

	if( p_dst->pD3DTexture )
	{
		p_dst->pD3DTexture->Release();
	}
	if( p_dst->pD3DPalette )
	{
		p_dst->pD3DPalette->Release();
	}

	D3DSURFACE_DESC	desc;
	uint32			num_levels = p_src->pD3DTexture->GetLevelCount();
	p_src->pD3DTexture->GetLevelDesc( 0, &desc );
	if( D3D_OK != D3DDevice_CreateTexture(	desc.Width,
											desc.Height,
											num_levels,
											0,
											desc.Format,
											0,
											&p_dst->pD3DTexture ))
	{
		exit( 0 );
	}

	// Create and copy the palette if present.
	if( p_src->pD3DPalette )
	{
		if( D3D_OK != D3DDevice_CreatePalette( D3DPALETTE_256, &p_dst->pD3DPalette ))
		{
			exit( 0 );
		}
		
		D3DCOLOR *p_src_palette;
		D3DCOLOR *p_dst_palette;
		p_src->pD3DPalette->Lock( &p_src_palette, D3DLOCK_READONLY );
		p_dst->pD3DPalette->Lock( &p_dst_palette, 0 );
		CopyMemory( p_dst_palette, p_src_palette, sizeof( D3DCOLOR ) * 256 );
	}
	
	for( uint32 l = 0; l < num_levels; ++l )
	{
		p_src->pD3DTexture->GetLevelDesc( l, &desc );

		D3DLOCKED_RECT src_rect, dst_rect;
		p_src->pD3DTexture->LockRect( l, &src_rect, NULL, D3DLOCK_READONLY );
		p_dst->pD3DTexture->LockRect( l, &dst_rect, NULL, 0 );

		CopyMemory( dst_rect.pBits, src_rect.pBits, desc.Size );
	}
	
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxTexture::plat_add_to_vram( void )
{								 	
	// Meaningless on Xbox, added to remove annoying debug stub output.
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxTexture::plat_remove_from_vram( void )
{								 	
	// Meaningless on Xbox, added to remove annoying debug stub output.
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint16	CXboxTexture::plat_get_width() const
{
	if( mp_texture )
		return mp_texture->ActualWidth;

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint16	CXboxTexture::plat_get_height() const
{
	if( mp_texture )
		return mp_texture->ActualHeight;

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CXboxTexture::plat_get_bitdepth() const
{
	if( mp_texture )
		return mp_texture->TexelDepth;

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CXboxTexture::plat_get_num_mipmaps() const
{
	return m_num_mipmaps;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CXboxTexture::plat_is_transparent() const
{
	return m_transparent;
}

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CTexDict

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CXboxTexDict::CXboxTexDict( uint32 checksum ) : CTexDict( checksum )
{
	// Load nothing
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CXboxTexDict::CXboxTexDict( const char *p_tex_dict_name ) : CTexDict( p_tex_dict_name, true )
{
	 LoadTextureDictionary( p_tex_dict_name );	// the derived class will does this
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxTexDict::~CXboxTexDict()
{
	UnloadTextureDictionary();				// the derived class does this
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxTexDict::LoadTextureDictionary( const char *p_tex_dict_name )
{
	// Count the number of entries in the lookup table. If it is empty, it is safe
	// to delete it and create a new, optimum sized one during the load process itself.
	if( mp_texture_lookup )
	{
		int num_items = 0;
		mp_texture_lookup->IterateStart();
		while( mp_texture_lookup->IterateNext())
			++num_items;

		if( num_items == 0 )
			mp_texture_lookup = Nx::LoadTextureFile( p_tex_dict_name, mp_texture_lookup, true );
		else
			Nx::LoadTextureFile( p_tex_dict_name, mp_texture_lookup );
	}
	else
	{
		Nx::LoadTextureFile( p_tex_dict_name, mp_texture_lookup );
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxTexDict::LoadTextureDictionaryFromMemory( void *p_mem )
{
	// Count the number of entries in the lookup table. If it is empty, it is safe
	// to delete it and create a new, optimum sized one during the load process itself.
	if( mp_texture_lookup )
	{
		int num_items = 0;
		mp_texture_lookup->IterateStart();
		while( mp_texture_lookup->IterateNext())
			++num_items;

		if( num_items == 0 )
			mp_texture_lookup =	Nx::LoadTextureFileFromMemory( &p_mem, mp_texture_lookup, true );
		else
			Nx::LoadTextureFileFromMemory( &p_mem, mp_texture_lookup );
	}
	else
	{
		Nx::LoadTextureFileFromMemory( &p_mem, mp_texture_lookup );
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxTexDict::UnloadTextureDictionary( void )
{
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CTexture *CXboxTexDict::plat_load_texture( const char *p_texture_name, bool sprite, bool alloc_vram )
{
	CXboxTexture *p_texture = new CXboxTexture;
	if( !p_texture->LoadTexture( p_texture_name, sprite ))
	{
		Dbg_Error("Can't load texture %s", p_texture_name);
	}
	return p_texture;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CTexture *CXboxTexDict::plat_reload_texture( const char *p_texture_name )
{
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxTexDict::plat_unload_texture( CTexture *p_texture )
{
	delete p_texture;

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxTexDict::plat_add_texture( CTexture *p_texture )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxTexDict::plat_remove_texture( CTexture *p_texture )
{
	return false;
}



#define MemoryRead( dst, size, num, src )	CopyMemory(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Lst::HashTable<Nx::CTexture>* LoadTextureFileFromMemory( void **pp_mem, Lst::HashTable<Nx::CTexture> *p_texture_table, bool okay_to_rebuild_texture_table )
{
	uint8 *p_data = (uint8*)( *pp_mem );

	// Read the texture file version and number of textures.
	int version, num_textures;
	MemoryRead( &version, sizeof( int ), 1, p_data );
	MemoryRead( &num_textures, sizeof( int ), 1, p_data );

	// If allowed, rebuild the texture table to the optimum size, using the same heap as the original table.
	if( okay_to_rebuild_texture_table )
	{
		uint32 optimal_table_size	= num_textures * 2;
		uint32 test					= 2;
		uint32 size					= 1;
		for( ;; test <<= 1, ++size )
		{
			// Check if this iteration of table size is sufficient, or if we have hit the maximum size.
			if(( optimal_table_size <= test ) || ( size >= 12 ))
			{
				Mem::Allocator::BlockHeader*	p_bheader	= Mem::Allocator::BlockHeader::sRead( p_texture_table );
				Mem::Allocator*					p_allocater	= p_bheader->mpAlloc;

				delete p_texture_table;

				Mem::Manager::sHandle().PushContext( p_allocater );
				p_texture_table = new Lst::HashTable<Nx::CTexture>( size );
				Mem::Manager::sHandle().PopContext();
				break;
			}
		}
	}

	for( int t = 0; t < num_textures; ++t )
	{
		// Create the engine level texture.
		NxXbox::sTexture *p_texture = new NxXbox::sTexture;

		uint32 base_width, base_height, levels, texel_depth, palette_depth, dxt, palette_size;
		MemoryRead( &p_texture->Checksum,	sizeof( uint32 ), 1, p_data );
		MemoryRead( &base_width,			sizeof( uint32 ), 1, p_data );
		MemoryRead( &base_height,			sizeof( uint32 ), 1, p_data );
		MemoryRead( &levels,				sizeof( uint32 ), 1, p_data );
		MemoryRead( &texel_depth,			sizeof( uint32 ), 1, p_data );
		MemoryRead( &palette_depth,			sizeof( uint32 ), 1, p_data );
		MemoryRead( &dxt,					sizeof( uint32 ), 1, p_data );
		MemoryRead( &palette_size,			sizeof( uint32 ), 1, p_data );

		p_texture->BaseWidth	= (uint16)base_width;
		p_texture->BaseHeight	= (uint16)base_height;
		p_texture->Levels		= (uint8)levels;
		p_texture->TexelDepth	= (uint8)texel_depth;
		p_texture->PaletteDepth	= (uint8)palette_depth;
		p_texture->DXT			= (uint8)dxt;
		
		D3DFORMAT	texture_format;
		if( p_texture->DXT > 0 )
		{
			if(( p_texture->DXT == 1 ) || ( p_texture->DXT == 2 ))
			{
				texture_format = D3DFMT_DXT1;
			}
			else if( p_texture->DXT == 5 )
			{
				texture_format = D3DFMT_DXT5;
			}
			else
			{
				Dbg_Assert( 0 );
			}
		}
		else if( p_texture->TexelDepth == 8 )
		{
			texture_format = D3DFMT_P8;
		}
		else if( p_texture->TexelDepth == 16 )
		{
			texture_format = D3DFMT_A1R5G5B5;	// Could also be X1R5G5B5;
		}
		else if( p_texture->TexelDepth == 32 )
		{
			texture_format = D3DFMT_A8R8G8B8;
		}
		else
		{
			Dbg_Assert( 0 );
		}
		
		if( D3D_OK != D3DDevice_CreateTexture(	p_texture->BaseWidth, p_texture->BaseHeight, p_texture->Levels,	0, texture_format, 0, &p_texture->pD3DTexture ))
		{
			Dbg_Assert( 0 );
		}

		if( palette_size > 0 )
		{
			// Create and lock the palette.
			if( D3D_OK != D3DDevice_CreatePalette(	palette_size == ( 256 * sizeof( D3DCOLOR )) ? D3DPALETTE_256 : D3DPALETTE_32, &p_texture->pD3DPalette ))
			{
				Dbg_Assert( 0 );
			}
			else
			{
				D3DCOLOR* p_colors;
				if( D3D_OK != p_texture->pD3DPalette->Lock( &p_colors, 0 ))
				{
					Dbg_Assert( 0 );
				}
				else
				{
					// Read in palette data.
					MemoryRead( p_colors, palette_size, 1, p_data );
				}
			}
		}
		else
		{
			p_texture->pD3DPalette = NULL;
		}

		for( uint32 mip_level = 0; mip_level < p_texture->Levels; ++mip_level )
		{
			uint32 texture_level_data_size;
			MemoryRead( &texture_level_data_size, sizeof( uint32 ), 1, p_data );

			D3DLOCKED_RECT locked_rect;
			if( D3D_OK != p_texture->pD3DTexture->LockRect( mip_level, &locked_rect, NULL, 0 ))
			{
				Dbg_Assert( 0 );
			}
			else
			{
				MemoryRead( locked_rect.pBits, texture_level_data_size, 1, p_data );
			}
		}

		// Add this texture to the table.
		Nx::CXboxTexture *p_xbox_texture = new Nx::CXboxTexture();
		p_xbox_texture->SetEngineTexture( p_texture );
		p_texture_table->PutItem( p_texture->Checksum, p_xbox_texture );
	}
	return p_texture_table;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Lst::HashTable<Nx::CTexture>* LoadTextureFile( const char *Filename, Lst::HashTable<Nx::CTexture> *p_texture_table, bool okay_to_rebuild_texture_table )
{
	// Open the texture file.
	void *p_FH = File::Open( Filename, "rb" );
	if( !p_FH )
	{
		Dbg_Message( "Couldn't open texture file %s\n", Filename );
		return p_texture_table;
	}

	// Read the texture file version and number of textures.
	int version, num_textures;
	File::Read( &version, sizeof( int ), 1, p_FH );
	File::Read( &num_textures, sizeof( int ), 1, p_FH );

	// If allowed, rebuild the texture table to the optimum size, using the same heap as the original table.
	if( okay_to_rebuild_texture_table )
	{
		uint32 optimal_table_size	= num_textures * 2;
		uint32 test					= 2;
		uint32 size					= 1;
		for( ;; test <<= 1, ++size )
		{
			// Check if this iteration of table size is sufficient, or if we have hit the maximum size.
			if(( optimal_table_size <= test ) || ( size >= 12 ))
			{
				Mem::Allocator::BlockHeader*	p_bheader	= Mem::Allocator::BlockHeader::sRead( p_texture_table );
				Mem::Allocator*					p_allocater	= p_bheader->mpAlloc;

				delete p_texture_table;

				Mem::Manager::sHandle().PushContext( p_allocater );
				p_texture_table = new Lst::HashTable<Nx::CTexture>( size );
				Mem::Manager::sHandle().PopContext();
				break;
			}
		}
	}

	for( int t = 0; t < num_textures; ++t )
	{
		// Create the engine level texture.
		NxXbox::sTexture *p_texture = new NxXbox::sTexture;

		uint32 base_width, base_height, levels, texel_depth, palette_depth, dxt, palette_size;
		File::Read( &p_texture->Checksum,	sizeof( uint32 ), 1, p_FH );
		File::Read( &base_width,			sizeof( uint32 ), 1, p_FH );
		File::Read( &base_height,			sizeof( uint32 ), 1, p_FH );
		File::Read( &levels,				sizeof( uint32 ), 1, p_FH );
		File::Read( &texel_depth,			sizeof( uint32 ), 1, p_FH );
		File::Read( &palette_depth,			sizeof( uint32 ), 1, p_FH );
		File::Read( &dxt,					sizeof( uint32 ), 1, p_FH );
		File::Read( &palette_size,			sizeof( uint32 ), 1, p_FH );

		p_texture->BaseWidth	= (uint16)base_width;
		p_texture->BaseHeight	= (uint16)base_height;
		p_texture->Levels		= (uint8)levels;
		p_texture->TexelDepth	= (uint8)texel_depth;
		p_texture->PaletteDepth	= (uint8)palette_depth;
		p_texture->DXT			= (uint8)dxt;
		
		D3DFORMAT	texture_format;
		if( p_texture->DXT > 0 )
		{
			if(( p_texture->DXT == 1 ) || ( p_texture->DXT == 2 ))
			{
				texture_format = D3DFMT_DXT1;
			}
			else if( p_texture->DXT == 5 )
			{
				texture_format = D3DFMT_DXT5;
			}
			else
			{
				Dbg_Assert( 0 );
			}
		}
		else if( p_texture->TexelDepth == 8 )
		{
			texture_format = D3DFMT_P8;
		}
		else if( p_texture->TexelDepth == 16 )
		{
			texture_format = D3DFMT_A1R5G5B5;	// Could also be X1R5G5B5;
		}
		else if( p_texture->TexelDepth == 32 )
		{
			texture_format = D3DFMT_A8R8G8B8;
		}
		else
		{
			Dbg_Assert( 0 );
		}
		
		if( D3D_OK != D3DDevice_CreateTexture(	p_texture->BaseWidth, p_texture->BaseHeight, p_texture->Levels,	0, texture_format, 0, &p_texture->pD3DTexture ))
		{
			Dbg_Assert( 0 );
		}

		if( palette_size > 0 )
		{
			// Create and lock the palette.
			if( D3D_OK != D3DDevice_CreatePalette(	palette_size == ( 256 * sizeof( D3DCOLOR )) ? D3DPALETTE_256 : D3DPALETTE_32, &p_texture->pD3DPalette ))
			{
				Dbg_Assert( 0 );
			}
			else
			{
				D3DCOLOR* p_colors;
				if( D3D_OK != p_texture->pD3DPalette->Lock( &p_colors, 0 ))
				{
					Dbg_Assert( 0 );
				}
				else
				{
					// Read in palette data.
					File::Read( p_colors, palette_size, 1, p_FH );
				}
			}
		}
		else
		{
			p_texture->pD3DPalette = NULL;
		}

		for( uint32 mip_level = 0; mip_level < p_texture->Levels; ++mip_level )
		{
			uint32 texture_level_data_size;
			File::Read( &texture_level_data_size, sizeof( uint32 ), 1, p_FH );

			D3DLOCKED_RECT locked_rect;
			if( D3D_OK != p_texture->pD3DTexture->LockRect( mip_level, &locked_rect, NULL, 0 ))
			{
				Dbg_Assert( 0 );
			}
			else
			{
				File::Read( locked_rect.pBits, texture_level_data_size, 1, p_FH );
			}
		}

		// Add this texture to the table.
		Nx::CXboxTexture *p_xbox_texture = new Nx::CXboxTexture();
		p_xbox_texture->SetEngineTexture( p_texture );
		p_texture_table->PutItem( p_texture->Checksum, p_xbox_texture );
	}
	File::Close( p_FH );

	return p_texture_table;
}


} // Namespace Nx  			
				
				
