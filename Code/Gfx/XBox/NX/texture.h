#ifndef __TEXTURE_H
#define __TEXTURE_H

#include <core/HashTable.h>

namespace NxXbox
{

struct sTexture
{
						sTexture();
						~sTexture();
						
	bool				SetRenderTarget( int width, int height, int depth, int z_depth );
	void				Set( int pass );

	uint32				Checksum;
	uint16				BaseWidth, BaseHeight;		// The size of the D3D texture (will be power of 2).
	uint16				ActualWidth, ActualHeight;	// The size of the texture itself (may not be power of 2).

	uint8				Levels;
	uint8				TexelDepth;
	uint8				PaletteDepth;
	uint8				DXT;

	IDirect3DTexture8*	pD3DTexture;
	IDirect3DPalette8*	pD3DPalette;
	IDirect3DSurface8*	pD3DSurface;
};

sTexture	*LoadTexture( const char *p_filename );

} // namespace NxXbox

#endif // __TEXTURE_H

