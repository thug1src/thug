#include <string.h>
#include <core/defines.h>
#include <core/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file/filesys.h>
#include "chars.h"
#include "texture.h"
#include "sys/ngc/p_gx.h"

namespace NxNgc
{

uint8*					TextureBuffer;
//sTexture* Textures		= NULL;
uint32 NumTextures		= 0;
uint32 NumTextureGroups	= 0;
Lst::HashTable< sTexture > *pTextureTable	= NULL;




sTexture::sTexture()
{
	pTexelData = NULL;
	pAlphaData = NULL;
	pOldAlphaData = NULL;
	flags = 0;
}

sTexture::~sTexture()
{
	if ( pTexelData )
	{
		delete pTexelData;
	}
	if ( pAlphaData && ( ( flags & TEXTURE_FLAG_CHANNEL_MASK ) == TEXTURE_FLAG_CHANNEL_GREEN ) )
	{
		delete pAlphaData;
	}
	if ( ( flags & TEXTURE_FLAG_OLD_DATA ) && pOldAlphaData )
	{
		delete pOldAlphaData;
	}
}



// Eeeek - the .img contains PS2 specific register values for bit depth.
// Use these values to convert them.
#define PSMCT32		0x00
#define PSMCT24		0x01
#define PSMCT16		0x02
#define PSMCT16S	0x0A
#define PS_GPU24	0x12
#define PSMT8		0x13
#define PSMT4		0x14
#define PSMT8H		0x1B
#define PSMT4HL		0x24
#define PSMT4HH		0x2C
#define PSMZ32		0x30
#define PSMZ24		0x31
#define PSMZ16		0x32
#define PSMZ16S		0x3A

const static uint32 TEXTURE_VERSION = 2;

sTexture *LoadTexture( const char *p_filename )
{
	sTexture *p_texture = NULL;

	void *p_FH = File::Open( p_filename, "rb" );

	if( p_FH )
	{
		uint32 version;
		uint32 checksum;
		uint32 width;
		uint32 height;
		uint32 depth;
		uint32 levels;
		uint32 rwidth;
		uint32 rheight;
		uint16 alphamap;
		uint16 hasholes;

		uint32 FileSize = File::GetFileSize(p_FH);

		// Load the actual texture

		p_texture = new sTexture();

		File::Read( &version,  4, 1, p_FH );
		File::Read( &checksum, 4, 1, p_FH );
		File::Read( &width,    4, 1, p_FH );
		File::Read( &height,   4, 1, p_FH );
		File::Read( &depth,    4, 1, p_FH );
		File::Read( &levels,   4, 1, p_FH );
		File::Read( &rwidth,   4, 1, p_FH );
		File::Read( &rheight,  4, 1, p_FH );
		File::Read( &alphamap, 2, 1, p_FH );
		File::Read( &hasholes, 2, 1, p_FH );

		Dbg_MsgAssert( version <= TEXTURE_VERSION, ("Illegal texture version number: %d (The newest version we deal with is: %d)\n", version, TEXTURE_VERSION ));
		Dbg_MsgAssert( depth == 32, ("We only deal with 32-bit textures.\n"));
	
		uint tsize = FileSize-(9*4);

		p_texture->flags = 0;
		p_texture->flags |= NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN;

		Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
		if ( tsize == ( width * height * 4 ) )
		{
			p_texture->format		= (uint8)GX_TF_RGBA8;
		
			p_texture->pTexelData = new uint8[tsize];
			p_texture->byte_size = tsize;
			Dbg_MsgAssert(p_texture->pTexelData, ("couldn't allocate buffer for texture file %s\n", p_filename));
			File::Read( p_texture->pTexelData, 1, tsize, p_FH );
		}
		else
		{
			p_texture->format		= (uint8)GX_TF_CMPR;
		
			if ( alphamap )
			{
				tsize = tsize / 2;
			}

			p_texture->pTexelData = new uint8[tsize];
			p_texture->byte_size = tsize;
			Dbg_MsgAssert(p_texture->pTexelData, ("couldn't allocate buffer for texture file %s\n", p_filename));
			File::Read( p_texture->pTexelData, 1, tsize, p_FH );

			if ( alphamap )
			{
				p_texture->pAlphaData = new uint8[tsize];
				Dbg_MsgAssert(p_texture->pAlphaData, ("couldn't allocate buffer for alpha file %s\n", p_filename));
				File::Read( p_texture->pAlphaData, 1, tsize, p_FH );
				p_texture->flags |= NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA;
			}
		}
		Mem::Manager::sHandle().BottomUpHeap()->PopAlign();

		p_texture->Checksum		= checksum;
		p_texture->BaseWidth	= width;
		p_texture->BaseHeight	= height;
		p_texture->Levels		= levels;
		p_texture->ActualWidth	= rwidth;
		p_texture->ActualHeight	= rheight;
		p_texture->flags |= hasholes ? NxNgc::sTexture::TEXTURE_FLAG_HAS_HOLES : 0;

		File::Close( p_FH );
	}
	
	return p_texture;
}





sTexture* GetTexture( uint32 checksum )
{
	if( pTextureTable )
	{
		pTextureTable->IterateStart();
		sTexture *p_tex = pTextureTable->IterateNext();
		while( p_tex )
		{
			if( p_tex->Checksum == checksum )
			{
				return p_tex;
			}
			p_tex = pTextureTable->IterateNext();
		}
	}
	return NULL;
}





bool sTexture::SetRenderTarget( int width, int height, int depth, int z_depth )
{
//	Checksum		= 0;;
//	BaseWidth		= width;
//	BaseHeight		= height;
//	ActualWidth		= width;
//	ActualHeight	= height;
//	Levels			= 0;
//	pAlphaData		= NULL;
//	format			= GX_TF_RGBA8;
//	
//	pTexelData		= new uint8[width*height*4];
//
	return true;
}

extern int g_id;

GXTexMapID sTexture::Upload( GXTexMapID id, uint8 flags, float k )
{
	GXTexMapID	rv = id;

	// Texels.
	GX::UploadTexture(  pTexelData,
						ActualWidth,
						ActualHeight,
						((GXTexFmt)format),
						flags & (1<<5) ? GX_CLAMP : GX_REPEAT,
						flags & (1<<6) ? GX_CLAMP : GX_REPEAT,
						Levels > 1 ? GX_TRUE : GX_FALSE,
						Levels > 1 ? GX_LIN_MIP_LIN : GX_LINEAR,
						GX_LINEAR,
						0.0f,
						Levels > 1 ? Levels - 1 : 0.0f,
						Levels > 1 ? k : 0.0f,
						GX_FALSE,
						GX_TRUE,
						GX_ANISO_1,
						rv ); 
	rv = (GXTexMapID)(((int)rv)+1);

	// Alpha.
	if ( flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA )
	{
		GX::UploadTexture(  pAlphaData,
							ActualWidth,
							ActualHeight,
							((GXTexFmt)format),
							flags & (1<<5) ? GX_CLAMP : GX_REPEAT,
							flags & (1<<6) ? GX_CLAMP : GX_REPEAT,
							Levels > 1 ? GX_TRUE : GX_FALSE,
							Levels > 1 ? GX_LIN_MIP_LIN : GX_LINEAR,
							GX_LINEAR,
							0.0f,
							Levels > 1 ? Levels - 1 : 0.0f,
							Levels > 1 ? k : 0.0f,
							GX_FALSE,
							GX_TRUE,
							GX_ANISO_1,
							rv ); 
		rv = (GXTexMapID)(((int)rv)+1);
	}

	return rv;
}


} // namespace NxNgc


