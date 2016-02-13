///////////////////////////////////////////////////////////////////////////////
// p_NxTexture.cpp

#include 	"Gfx/Nx.h"
#include 	"Gfx/Ngc/p_NxTexture.h"
#include 	"Gfx/Ngc/NX/import.h"

extern bool g_in_cutscene;

namespace Nx
{

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CTexture

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcTexture::CNgcTexture() :  mp_texture( NULL )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcTexture::~CNgcTexture()
{
	if( mp_texture )
	{
		delete mp_texture;
	}
}


/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcTexture::plat_load_texture( const char *p_texture_name, bool sprite, bool alloc_vram )
{
	char filename[256];

	strcpy( filename, p_texture_name );
	
	// append '.img.ngc' to the end.
	strcat( filename, ".img.ngc" );
	
	mp_texture = NxNgc::LoadTexture( filename );
	
	return mp_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcTexture::plat_replace_texture( CTexture *p_texture )
{						  
	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );

	Dbg_Assert(p_texture);
	
	CNgcTexture *p_Ngc_texture = static_cast<CNgcTexture *>( p_texture );

	// Go through and copy the texture.
	NxNgc::sTexture *p_src	= p_Ngc_texture->GetEngineTexture();
	NxNgc::sTexture *p_dst	= GetEngineTexture();

	// Couple of problem cases.
//	Dbg_MsgAssert( p_src->format == p_dst->format, ( "Cannot replace textures of different formats.\n" ));
//	if ( p_src->pAlphaData && !p_dst->pAlphaData )
//	{
//		Dbg_MsgAssert( false, ( "Cannot assign a texture with alpha to a texture without alpha.\n" ));
//	}
//	if ( !p_src->pAlphaData && p_dst->pAlphaData )
//	{
//		Dbg_MsgAssert( false, ( "Cannot assign a texture without alpha to a texture with alpha.\n" ));
//	}

	// Delete & re-allocate alpha space if new image is bigger, or not single owner alpha, or not channel 0.
	if ( p_src->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA )
	{
		// See if this texture needs to be stored in pOldAlpha.
		if ( !( p_dst->flags & NxNgc::sTexture::TEXTURE_FLAG_SINGLE_OWNER ) &&
			 ( ( p_dst->flags & NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_MASK ) == NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN ) )
		{
			p_dst->pOldAlphaData = p_dst->pAlphaData;
			p_dst->flags |= NxNgc::sTexture::TEXTURE_FLAG_OLD_DATA;
		}

		if ( ( p_dst->byte_size < p_src->byte_size ) ||
			 !( p_dst->flags & NxNgc::sTexture::TEXTURE_FLAG_SINGLE_OWNER ) ||
			 ( ( p_dst->flags & NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_MASK ) != NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN ) )
		{
			if( p_dst->pAlphaData &&
				( p_dst->byte_size < p_src->byte_size ) &&
				( p_dst->flags & NxNgc::sTexture::TEXTURE_FLAG_SINGLE_OWNER ) &&
				( ( p_dst->flags & NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_MASK ) == NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN ) )
			{
				delete [] p_dst->pAlphaData;
			}
			p_dst->pAlphaData = new uint8[p_src->byte_size];
		}
	}

	// Delete & re-allocate texture space if new image is bigger.
	if ( p_dst->byte_size < p_src->byte_size )
	{
		if( p_dst->pTexelData )
		{
			delete [] p_dst->pTexelData;
		}
		p_dst->pTexelData = new uint8[p_src->byte_size];
		p_dst->byte_size = p_src->byte_size;
	}

	p_dst->BaseWidth	= p_src->BaseWidth;
	p_dst->BaseHeight	= p_src->BaseHeight;
	p_dst->ActualWidth	= p_src->ActualWidth;
	p_dst->ActualHeight	= p_src->ActualHeight;

	p_dst->flags		= ( p_dst->flags & ~NxNgc::sTexture::TEXTURE_FLAG_HAS_HOLES ) | ( p_src->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_HOLES );

	// Copy the pixel data.
	memcpy( p_dst->pTexelData, p_src->pTexelData, p_src->byte_size );
	DCFlushRange ( p_dst->pTexelData, p_src->byte_size );

	// Copy the alpha data if necessary.
	if ( p_src->pAlphaData && p_dst->pAlphaData )
	{
		memcpy( p_dst->pAlphaData, p_src->pAlphaData, p_src->byte_size );
		DCFlushRange ( p_dst->pAlphaData, p_src->byte_size );
		p_dst->flags		= ( p_dst->flags & ~NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA ) | ( p_src->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA );
		p_dst->flags		= ( p_dst->flags & ~NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_MASK ) | NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN;
		p_dst->flags		= p_dst->flags | NxNgc::sTexture::TEXTURE_FLAG_SINGLE_OWNER;
	}

	// If the replacement texture doesn't have alpha, but the original does, twiddle the flag.
	if ( p_dst->pAlphaData && !p_src->pAlphaData )
	{
		p_dst->flags		= ( p_dst->flags & ~NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA );
	}
	
	// Replacement texture does not mip-map.
	p_dst->Levels = 0;

	// So that if the regular texture is 32-bit, it can have a CMPR texture copied over it.
	p_dst->format		= p_src->format;

	// Flag this texture as having been replaced.
	p_dst->flags |= NxNgc::sTexture::TEXTURE_FLAG_REPLACED;

	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcTexture::plat_add_to_vram( void )
{								 	
	// Meaningless on Ngc, added to remove annoying debug stub output.
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcTexture::plat_remove_from_vram( void )
{								 	
	// Meaningless on Ngc, added to remove annoying debug stub output.
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint16	CNgcTexture::plat_get_width() const
{
	if( mp_texture )
	{
		return mp_texture->BaseWidth;
	}
	else
	{
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint16	CNgcTexture::plat_get_height() const
{
	if( mp_texture )
	{
		return mp_texture->BaseHeight;
	}
	else
	{
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CNgcTexture::plat_get_bitdepth() const
{
	switch ( ((GXTexFmt)mp_texture->format) )
	{
		case GX_TF_I4:
		case GX_TF_IA4:
		case GX_TF_CMPR:
			return 4;
			break;

		case GX_TF_I8:
		case GX_TF_IA8:
		case GX_TF_A8:
		case GX_TF_Z8:
			return 8;
			break;
		
		case GX_TF_RGB565:
		case GX_TF_RGB5A3:
		case GX_TF_Z16:
			return 16;
			break;

		default:
		case GX_TF_RGBA8:
		case GX_TF_Z24X8:
			return 32;
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CNgcTexture::plat_get_num_mipmaps() const
{
	return mp_texture->Levels;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CNgcTexture::plat_is_transparent() const
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CTexDict

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CNgcTexDict::CNgcTexDict( uint32 checksum ) : CTexDict( checksum )
{
	// Load nothing
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CNgcTexDict::CNgcTexDict( const char *p_tex_dict_name, bool forceTexDictLookup ) : CTexDict( p_tex_dict_name, true )
{
	 LoadTextureDictionary( p_tex_dict_name, forceTexDictLookup );	// the derived class will does this
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcTexDict::~CNgcTexDict()
{
	UnloadTextureDictionary();				// the derived class does this
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcTexDict::LoadTextureDictionary( const char *p_tex_dict_name, bool forceTexDictLookup )
{
	NxNgc::LoadTextureFile( p_tex_dict_name, mp_texture_lookup );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcTexDict::LoadTextureDictionaryFromMemory( void *p_mem )
{
	Nx::LoadTextureFileFromMemory( &p_mem, mp_texture_lookup );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcTexDict::UnloadTextureDictionary()
{
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CTexture *CNgcTexDict::plat_load_texture(const char *p_texture_name, bool sprite, bool alloc_vram )
{
	CNgcTexture *p_texture = new CNgcTexture;
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
CTexture *CNgcTexDict::plat_reload_texture( const char *p_texture_name )
{
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcTexDict::plat_unload_texture( CTexture *p_texture )
{
	delete p_texture;

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcTexDict::plat_add_texture( CTexture *p_texture )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcTexDict::plat_remove_texture( CTexture *p_texture )
{
	return false;
}



#define MemoryRead( dst, size, num, src )	memcpy(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void LoadTextureFileFromMemory( void **pp_mem, Lst::HashTable<Nx::CTexture> *p_texture_table )
{
	uint8 *p_data = (uint8*)( *pp_mem );

	uint32 temp;
	
	// Open the texture file.
//	void *p_FH = File::Open( Filename, "rb" );
//	if( !p_FH )
//	{
//		Dbg_Message( "Couldn't open texture file %s\n", Filename );
////		return 0;
//		return;
//	}

	// Read the texture file version.
	int version;
	MemoryRead( &version, sizeof( int ), 1, p_data );

	// Read the number of textures.	
	int num_textures;
	MemoryRead( &num_textures, sizeof( int ), 1, p_data );
	
//	NumTextures				+= num_textures;

	int bytes;
	bool need_to_pop;

	NxNgc::sTexture ** p_tex = new (Mem::Manager::sHandle().TopDownHeap()) NxNgc::sTexture *[num_textures];
	unsigned short * p_resolvem = new (Mem::Manager::sHandle().TopDownHeap()) unsigned short[num_textures];
	unsigned short * p_resolvec = new (Mem::Manager::sHandle().TopDownHeap()) unsigned short[num_textures];
	int num_resolve = 0;

	int mem_available;
	for( int t = 0; t < num_textures; ++t )
	{
		need_to_pop = false;
		bytes = sizeof( NxNgc::sTexture );
		if ( g_in_cutscene )
		{
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
			mem_available = Mem::Manager::sHandle().Available();
			if ( bytes < ( mem_available - ( 40 * 1024 ) ) )
			{
				need_to_pop = true;
			}
			else
			{
				Mem::Manager::sHandle().PopContext();
				Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
				mem_available = Mem::Manager::sHandle().Available();
				if ( bytes < ( mem_available - ( 5 * 1024 ) ) )
				{
					need_to_pop = true;
				}
				else
				{
					Mem::Manager::sHandle().PopContext();
					Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
					mem_available = Mem::Manager::sHandle().Available();
					if ( bytes < ( mem_available - ( 40 * 1024 ) ) )
					{
						need_to_pop = true;
					}
					else
					{
						Mem::Manager::sHandle().PopContext();
					}
				}
			}
		}

		NxNgc::sTexture *p_texture = new NxNgc::sTexture;
		
		if ( need_to_pop )
		{
			Mem::Manager::sHandle().PopContext();
		}

		p_tex[t] = p_texture;

		MemoryRead( &p_texture->Checksum,		sizeof( uint32 ), 1, p_data );
		MemoryRead( &temp,		sizeof( uint32 ), 1, p_data );
		p_texture->BaseWidth = (uint16)temp;
		MemoryRead( &temp,		sizeof( uint32 ), 1, p_data );
		p_texture->BaseHeight = (uint16)temp;
		MemoryRead( &temp,		sizeof( uint32 ), 1, p_data );
		p_texture->Levels = (uint16)temp;

		p_texture->ActualWidth = ( p_texture->BaseWidth + 3 ) & ~3;
		p_texture->ActualHeight = ( p_texture->BaseHeight + 3 ) & ~3;

		int tex_format;
		int channel;
		int index;
		MemoryRead( &tex_format, sizeof( uint32 ), 1, p_data );
		channel = ( tex_format >> 8 ) & 0xff;
		index = ( tex_format >> 16 ) & 0xffff;
		tex_format = tex_format & 0xff;

		switch ( tex_format ) {
			case 0:
				p_texture->format = GX_TF_CMPR;
				break;
			case 1:
				p_texture->format = GX_TF_CMPR;
				break;
			case 2:
				p_texture->format = GX_TF_RGBA8;
				break;
			default:
				Dbg_MsgAssert( false, ("Illegal texture format: %d\n", tex_format ));
				break;
		}

		p_texture->flags = 0;

		int has_holes;
		MemoryRead( &has_holes, sizeof( uint32 ), 1, p_data );
		p_texture->flags |= has_holes ? NxNgc::sTexture::TEXTURE_FLAG_HAS_HOLES : 0;

		uint8 * p8[8];
		int mipbytes[8];

		// Read color maps.
		bytes = 0;
		for( uint32 mip_level = 0; mip_level < p_texture->Levels; ++mip_level )
		{
			uint32 texture_level_data_size;
			MemoryRead( &texture_level_data_size,			sizeof( uint32 ), 1, p_data );

			p8[mip_level] = new (Mem::Manager::sHandle().TopDownHeap()) uint8[texture_level_data_size];
			mipbytes[mip_level] = texture_level_data_size;
			bytes += texture_level_data_size;

			MemoryRead( p8[mip_level], texture_level_data_size, 1, p_data );
		}
		// Copy all textures & delete the originals.

		need_to_pop = false;
		if ( g_in_cutscene )
		{
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
			mem_available = Mem::Manager::sHandle().Available();
			if ( bytes < ( mem_available - ( 40 * 1024 ) ) )
			{
				need_to_pop = true;
			}
			else
			{
				Mem::Manager::sHandle().PopContext();
				Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
				mem_available = Mem::Manager::sHandle().Available();
				if ( bytes < ( mem_available - ( 5 * 1024 ) ) )
				{
					need_to_pop = true;
				}
				else
				{
					Mem::Manager::sHandle().PopContext();
					Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
					mem_available = Mem::Manager::sHandle().Available();
					if ( bytes < ( mem_available - ( 40 * 1024 ) ) )
					{
						need_to_pop = true;
					}
					else
					{
						Mem::Manager::sHandle().PopContext();
					}
				}
			}
		}

		Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
		p_texture->pTexelData = new uint8[bytes];
		p_texture->byte_size = bytes;
		Mem::Manager::sHandle().BottomUpHeap()->PopAlign();

		if ( need_to_pop )
		{
			Mem::Manager::sHandle().PopContext();
		}

		uint8 * pTex = p_texture->pTexelData;
		for( uint32 mip_level = 0; mip_level < p_texture->Levels; ++mip_level )
		{
			memcpy( pTex, p8[mip_level], mipbytes[mip_level] );
			delete p8[mip_level];
			pTex += mipbytes[mip_level];
		}

		// Read alpha maps.
		if ( tex_format == 1 )
		{
			if ( channel == 0 )
			{
				bytes = 0;
				for( uint32 mip_level = 0; mip_level < p_texture->Levels; ++mip_level )
				{
					uint32 texture_level_data_size;
					MemoryRead( &texture_level_data_size,			sizeof( uint32 ), 1, p_data );

					p8[mip_level] = new (Mem::Manager::sHandle().TopDownHeap()) uint8[texture_level_data_size];
					mipbytes[mip_level] = texture_level_data_size;
					bytes += texture_level_data_size;

					MemoryRead( p8[mip_level], texture_level_data_size, 1, p_data );
				}
				// Copy all textures & delete the originals.

				need_to_pop = false;
				if ( g_in_cutscene )
				{
					Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
					mem_available = Mem::Manager::sHandle().Available();
					if ( bytes < ( mem_available - ( 40 * 1024 ) ) )
					{
						need_to_pop = true;
					}
					else
					{
						Mem::Manager::sHandle().PopContext();
						Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
						mem_available = Mem::Manager::sHandle().Available();
						if ( bytes < ( mem_available - ( 5 * 1024 ) ) )
						{
							need_to_pop = true;
						}
						else
						{
							Mem::Manager::sHandle().PopContext();
							Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
							mem_available = Mem::Manager::sHandle().Available();
							if ( bytes < ( mem_available - ( 40 * 1024 ) ) )
							{
								need_to_pop = true;
							}
							else
							{
								Mem::Manager::sHandle().PopContext();
							}
						}
					}
				}

				Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
				p_texture->pAlphaData = new uint8[bytes];
				Mem::Manager::sHandle().BottomUpHeap()->PopAlign();

				if ( need_to_pop )
				{
					Mem::Manager::sHandle().PopContext();
				}

				uint8 * pTex = p_texture->pAlphaData;
				for( uint32 mip_level = 0; mip_level < p_texture->Levels; ++mip_level )
				{
					memcpy( pTex, p8[mip_level], mipbytes[mip_level] );
					delete p8[mip_level];
					pTex += mipbytes[mip_level];
				}
				p_texture->flags |= NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA;
			}
			else
			{
				p_texture->flags |= NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA;

				p_resolvem[num_resolve] = t;
				p_resolvec[num_resolve] = index;
				num_resolve++;
			}
			switch ( channel )
			{
				case 0:
				default:
					p_texture->flags |= NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN;
					break;
				case 1:
					p_texture->flags |= NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_RED;
					break;
				case 2:
					p_texture->flags |= NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_BLUE;
					break;
			}
			p_texture->flags |= NxNgc::sTexture::TEXTURE_FLAG_SINGLE_OWNER;
		}
		else
		{
			// No unique alpha map.
			p_texture->pAlphaData = NULL;
			p_texture->flags &= ~NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA;
			p_texture->flags |= NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN;
		}

		// Add this texture to the table.
//		pTextureTable->PutItem( p_texture->Checksum, p_texture );
		need_to_pop = false;

		bytes = sizeof( Nx::CNgcTexture );
		if ( g_in_cutscene )
		{
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
			mem_available = Mem::Manager::sHandle().Available();
			if ( bytes < ( mem_available - ( 40 * 1024 ) ) )
			{
				need_to_pop = true;
			}
			else
			{
				Mem::Manager::sHandle().PopContext();
				Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
				mem_available = Mem::Manager::sHandle().Available();
				if ( bytes < ( mem_available - ( 5 * 1024 ) ) )
				{
					need_to_pop = true;
				}
				else
				{
					Mem::Manager::sHandle().PopContext();
					Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
					mem_available = Mem::Manager::sHandle().Available();
					if ( bytes < ( mem_available - ( 40 * 1024 ) ) )
					{
						need_to_pop = true;
					}
					else
					{
						Mem::Manager::sHandle().PopContext();
					}
				}
			}
		}

		Nx::CNgcTexture *p_Ngc_texture = new Nx::CNgcTexture();

		if ( need_to_pop )
		{
			Mem::Manager::sHandle().PopContext();
		}

		p_Ngc_texture->SetEngineTexture( p_texture );
		if ( p_texture_table )
		{
			p_texture_table->PutItem( p_texture->Checksum, p_Ngc_texture );
		}
	}

	// Resolve alpha maps.
	for ( int r = 0; r < num_resolve; r++ )
	{
		p_tex[p_resolvem[r]]->pAlphaData = p_tex[p_resolvec[r]]->pAlphaData;
		p_tex[p_resolvec[r]]->flags &= ~NxNgc::sTexture::TEXTURE_FLAG_SINGLE_OWNER;
	}

	delete p_tex;
	delete p_resolvem;
	delete p_resolvec;
//	return pTextureTable;
}

} // Namespace Nx  			
				
				

