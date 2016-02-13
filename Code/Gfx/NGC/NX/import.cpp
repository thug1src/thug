#include <core/defines.h>
#include <sys/file/filesys.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "texture.h"
#include "gfx/ngc/p_nxtexture.h"
#include "mesh.h"
#include "import.h"
	
namespace NxNgc
{

uint8 *pData, *TextureLoadPos;

//----------------------------------------------------------------------------------------
//		L O A D   T E X T U R E   F I L E
//----------------------------------------------------------------------------------------

void LoadTextureFile( const char* Filename, Lst::HashTable<Nx::CTexture> * p_texture_table )
{
	uint32 temp;
	
	Dbg_Message( "Loading texture file %s\n", Filename );

	// Open the texture file.
	void *p_FH = File::Open( Filename, "rb" );
	if( !p_FH )
	{
		Dbg_Message( "Couldn't open texture file %s\n", Filename );
//		return 0;
		return;
	}

	// Read the texture file version.
	int version;
	File::Read( &version, sizeof( int ), 1, p_FH );

	// Read the number of textures.	
	int num_textures;
	File::Read( &num_textures, sizeof( int ), 1, p_FH );
	
//	NumTextures				+= num_textures;

	NxNgc::sTexture ** p_tex = new (Mem::Manager::sHandle().TopDownHeap()) NxNgc::sTexture *[num_textures];
	unsigned short * p_resolvem = new (Mem::Manager::sHandle().TopDownHeap()) unsigned short[num_textures];
	unsigned short * p_resolvec = new (Mem::Manager::sHandle().TopDownHeap()) unsigned short[num_textures];
	int num_resolve = 0;

	for( int t = 0; t < num_textures; ++t )
	{
		sTexture *p_texture = new sTexture;
		
		p_tex[t] = p_texture;

		File::Read( &p_texture->Checksum,		sizeof( uint32 ), 1, p_FH );
		File::Read( &temp,		sizeof( uint32 ), 1, p_FH );
		p_texture->BaseWidth = (uint16)temp;
		File::Read( &temp,		sizeof( uint32 ), 1, p_FH );
		p_texture->BaseHeight = (uint16)temp;
		File::Read( &temp,		sizeof( uint32 ), 1, p_FH );
		p_texture->Levels = (uint16)temp;

		p_texture->ActualWidth = ( p_texture->BaseWidth + 3 ) & ~3;
		p_texture->ActualHeight = ( p_texture->BaseHeight + 3 ) & ~3;

		int tex_format;
		int channel;
		int index;
		File::Read( &tex_format, sizeof( uint32 ), 1, p_FH );
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
		File::Read( &has_holes, sizeof( uint32 ), 1, p_FH );
		p_texture->flags |= has_holes ? NxNgc::sTexture::TEXTURE_FLAG_HAS_HOLES : 0;

		uint8 * p8[8];
		int bytes;
		int mipbytes[8];

		// Read color maps.
		bytes = 0;
		for( uint32 mip_level = 0; mip_level < p_texture->Levels; ++mip_level )
		{
			uint32 texture_level_data_size;
			File::Read( &texture_level_data_size,			sizeof( uint32 ), 1, p_FH );

			p8[mip_level] = new (Mem::Manager::sHandle().TopDownHeap()) uint8[texture_level_data_size];
			mipbytes[mip_level] = texture_level_data_size;
			bytes += texture_level_data_size;

			File::Read( p8[mip_level], texture_level_data_size, 1, p_FH );
		}
		// Copy all textures & delete the originals.
		Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
		p_texture->pTexelData = new uint8[bytes];
		p_texture->byte_size = bytes;
		Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
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
					File::Read( &texture_level_data_size,			sizeof( uint32 ), 1, p_FH );

					p8[mip_level] = new (Mem::Manager::sHandle().TopDownHeap()) uint8[texture_level_data_size];
					mipbytes[mip_level] = texture_level_data_size;
					bytes += texture_level_data_size;

					File::Read( p8[mip_level], texture_level_data_size, 1, p_FH );
				}
				// Copy all textures & delete the originals.
				Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
				p_texture->pAlphaData = new uint8[bytes];
				Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
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
		Nx::CNgcTexture *p_Ngc_texture = new Nx::CNgcTexture();
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

	File::Close( p_FH );

	delete p_tex;
	delete p_resolvem;
	delete p_resolvec;

//	return pTextureTable;
}




//void Error(int err)
//{
//	printf("Error = %d\n", err);
//	exit(1);
//}

} // namespace NxNgc

