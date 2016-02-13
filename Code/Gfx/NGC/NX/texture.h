#ifndef __TEXTURE_H
#define __TEXTURE_H

#include <core/HashTable.h>
#include <dolphin.h>

namespace NxNgc
{

struct sTexture
{
	enum ETextureFlags
	{
		TEXTURE_FLAG_HAS_HOLES				= (1<<0),
		TEXTURE_FLAG_HAS_ALPHA				= (1<<1),
		TEXTURE_FLAG_CHANNEL_GREEN			= (1<<2),
		TEXTURE_FLAG_CHANNEL_RED			= (1<<3),
		TEXTURE_FLAG_CHANNEL_BLUE			= (1<<4),
		TEXTURE_FLAG_SINGLE_OWNER			= (1<<5),
		TEXTURE_FLAG_OLD_DATA				= (1<<6),
		TEXTURE_FLAG_REPLACED				= (1<<7),

		TEXTURE_FLAG_CHANNEL_MASK			= ( TEXTURE_FLAG_CHANNEL_GREEN | TEXTURE_FLAG_CHANNEL_RED | TEXTURE_FLAG_CHANNEL_BLUE )
	};

						sTexture();
						~sTexture();
	
	bool				SetRenderTarget( int width, int height, int depth, int z_depth );

	GXTexMapID			Upload( GXTexMapID id, uint8 flags, float k );

	uint32				Checksum;
	uint16				BaseWidth, BaseHeight;		// The size of the D3D texture (will be power of 2).
	uint16				ActualWidth, ActualHeight;	// The size of the texture itself (may not be power of 2).
	uint8				Levels;
//	uint8				HasHoles;
	uint8				format;
//	uint8				has_alpha;
//	uint16				channel;
//	uint16				single_owner;			// 1 means single owner.
	uint16				flags;
	uint32				byte_size;
	uint8*				pTexelData;
	uint8*				pAlphaData;
	uint8*				pOldAlphaData;
};


sTexture* GetTexture( uint32 checksum );

sTexture	*LoadTexture( const char *p_filename );
void LoadTextureGroup(int Group);

extern uint8 *TextureBuffer;
//extern sTexture *Textures;
extern Lst::HashTable< sTexture > *pTextureTable;

extern uint32 NumTextures, NumTextureGroups;
extern uint NextTBP, LastCBP;

} // namespace NxNgc

#endif // __TEXTURE_H


