#include <stdlib.h>
#include <string.h>

#include <core/defines.h>
#include <core/debug.h>
#include <core/string/stringutils.h>
#include <core/macros.h>
#include <gfx/nxfontman.h>
#include <sys/config/config.h>
#include <sys/file/filesys.h>

#include "nx_init.h"
#include "render.h"
#include "chars.h"
#include "xbmemfnt.h"


/*


	
.fnt file format (by Ryan)
--------------------------

	4	File size in bytes
	4	Number of characters
	4	Default height
	4	Default base
	
	?	Character table (see below)
	?	Texture (see below)

	Character
	2	Baseline (how many pixels down relative to top of image)
	2	Ascii value

	Texture
	4	Size of texture
	2	Width
	2	Height
	2	Bit depth
	6	Padding
	W*H	Raw data
	0-3	Padding for uint32 alignment
	1K	Palette data
	4	Number of subtextures
	?	Subtexture table (see below)

	Subtexture
	2	X
	2	Y
	2	W
	2	H
	
*/


namespace NxXbox
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
typedef struct
{
	float		x, y, z;
	float		rhw;
	D3DCOLOR	col;
	float		u, v;
}
sFontVert;


SFont			*pFontList;
SFont			*pButtonsFont				= NULL;
SFont			*SText::spOverrideFont		= NULL;

const uint32	CHARS_PER_BUFFER			= 256;
static int		font_vertex_offset			= 0;
static BYTE*	p_locked_font_vertex_buffer	= NULL;
static BYTE		font_vertex_buffer[CHARS_PER_BUFFER * 4 * sizeof( sFontVert )];

static uint32	swizzle_table[4096];
static bool		swizzle_table_generated		= false;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void generateSwizzleTable( void )
{
	if( !swizzle_table_generated )
	{
		for( uint32 i = 0, value = 0; i < 4096; i++ )
		{
			swizzle_table[i] = value;
			value += 0x2AAAAAAB;
			value &= 0x55555555;
		}
		swizzle_table_generated = true;
	}
}



#define TWIDDLE(_u, _v) ((swizzle_table[(_v)] << 1) | (swizzle_table[(_u)]))


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SwizzleTexture( void *dstBuffer, void *srcBuffer, int width, int height, int32 depth, int32 stride )
{
    int32 tilesX, tilesY;
    int32 tilesSizeX, tilesSizeY;
    int32 tileSize;

	generateSwizzleTable();

	if( width > height )
    {
        tilesX = width / height;
        tilesY = 1;

        tilesSizeX = width / tilesX;
        tilesSizeY = height;
    }
    else
    {
        tilesX = 1;
        tilesY = height / width;

        tilesSizeX = width;
        tilesSizeY = height / tilesY;
    }

    tileSize = tilesSizeX * tilesSizeY;

	switch (depth)
	{
		case 4:
	    case 8:
        {
            int32 j;

            for (j = 0; j < tilesY; j++)
            {
                int32 i;

                for (i = 0; i < tilesX; i++)
                {
            int32 y;
                    uint8 *base;

                    base = (uint8 *)(((uint8 *)dstBuffer) +
                                       ((tileSize * tilesX) * j) +
                                       (tileSize * i));

                    for (y = 0; y < tilesSizeY; y++)
            {
                uint8    *srcPixel;
                int32     x;

                        srcPixel = (uint8 *)(((uint8 *)srcBuffer) +
                                               (stride * (tilesSizeY * j)) +
                                               (tilesSizeX * i) +
                                               (stride * y));

                        for (x = 0; x < tilesSizeX; x++)
                {
                    uint8    *dstPixel;
                        dstPixel = (uint8 *)(base + TWIDDLE(x, y));
		                *dstPixel = *srcPixel;

                    srcPixel++;
                }
            }
        }
            }
        }
        break;

    case 16:
        {
            int32 j;

            for (j = 0; j < tilesY; j++)
            {
                int32 i;

                for (i = 0; i < tilesX; i++)
                {
            int32 y;
                    uint8 *base;

                    base = (uint8 *)(((uint16 *)dstBuffer) +
                                       ((tileSize * tilesX) * j) +
                                       (tileSize * i));

                    for (y = 0; y < tilesSizeY; y++)
            {
                uint16    *srcPixel;
                int32     x;

                        srcPixel = (uint16 *)(((uint8 *)srcBuffer) +
                                                (stride * (tilesSizeY * j)) +
                                                (2 * tilesSizeX * i) +
                                                (stride * y));

                        for (x = 0; x < tilesSizeX; x++)
                {
                    uint16    *dstPixel;
                    dstPixel = (uint16 *)(base + (TWIDDLE(x, y) << 1));
                    *dstPixel = *srcPixel;

                    srcPixel++;
                }
            }
        }
            }
        }
        break;

    case 24:
    case 32:
        {
            int32 j;

            for (j = 0; j < tilesY; j++)
            {
                int32 i;

                for (i = 0; i < tilesX; i++)
                {
            int32 y;
                    uint8 *base;

                    base = (uint8 *)(((uint32 *)dstBuffer) +
                                       ((tileSize * tilesX) * j) +
                                       (tileSize * i));

                    for (y = 0; y < tilesSizeY; y++)
            {
                uint32    *srcPixel;
                int32     x;

                        srcPixel = (uint32 *)(((uint8 *)srcBuffer) +
                                                (stride * (tilesSizeY * j)) +
                                                (4 * tilesSizeX * i) +
                                                (stride * y));

                        for (x = 0; x < tilesSizeX; x++)
                {
                    uint32    *dstPixel;
                    dstPixel = (uint32 *)(base + (TWIDDLE(x, y) << 2));
                    *dstPixel = *srcPixel;

                    srcPixel++;
                }
            }
        }
            }
        }
        break;

    default:
		exit( 0 );
        break;
    }
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
SFont* InitialiseMemoryResidentFont( void )
{
	return LoadFont((const char*)xbmemfnt, true );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
SFont* LoadFont( const char *Filename, bool memory_resident )
{
	SFont*	pFont;
	SChar*	pChar;
	uint8*	pData;
	void*	p_FH;
	int		i,Len,NumChars,Width,Height,Depth,NumBytes;

	// Build the full filename.
	char filename[128];

	if( !memory_resident )
	{
		strcpy( filename, "fonts/" );
		strcat( filename, Filename );
		strcat( filename, ".fnt.xbx" );
	}
	
	// Open the font file.
	if( !memory_resident )
		p_FH = File::Open( filename, "rb" );

	// Allocate memory for the font structure.
	pFont = new SFont();

	// Allocate a temporary buffer.
	uint8 FontBuf[2048];

	// Load file header.
	if( !memory_resident )
	{
		Len = File::Read( FontBuf, 16, 1, p_FH );
		Dbg_MsgAssert( Len == 16, ( "couldn't read file header from font file %s", Filename ));
	}
	else
	{
		CopyMemory( FontBuf, Filename, 16 );
		Filename += 16;
	}

	NumChars			 = ((uint32 *)FontBuf)[1];
	pFont->DefaultHeight = ((uint32 *)FontBuf)[2];
	pFont->DefaultBase	 = ((uint32 *)FontBuf)[3];

	// Clear character map to zero.
	memset( pFont->Map, 0, 256 );
	memset( pFont->SpecialMap, 0, 32 );

	// Allocate memory for character table.
	pFont->pChars = new SChar[NumChars];

	// Load character map and character table.
	if( !memory_resident )
	{
		Len = File::Read( FontBuf, NumChars << 2, 1, p_FH );
		Dbg_MsgAssert( Len == ( NumChars << 2 ), ( "couldn't read character table in font file %s", Filename ));
	}
	else
	{
		CopyMemory( FontBuf, Filename, NumChars << 2 );
		Filename += NumChars << 2;
	}

	for( i = 0, pChar = pFont->pChars, pData = FontBuf; i < NumChars; i++,pChar++,pData += 4 )
	{
		pChar->Baseline							= ((uint16 *)pData)[0];
		sint16 ascii_val = ((sint16 *)pData)[1];
		if (ascii_val >= 0)
			pFont->Map[(uint8) ascii_val] = i;
		else
		{
			Dbg_Assert(ascii_val >= -32)
			pFont->SpecialMap[(uint8) (-ascii_val - 1)] = i;
		}
	}

	// If there is a null character in the font, make characters that could not be found
	// in the font display that instead of 'A'
	if( pFont->SpecialMap[31] != 0 )
	{
		for( i = 0; i < 256; ++i ) 
		{
			if( pFont->Map[i] == 0 && i != 'A' && i != 'a')
				pFont->Map[i] = pFont->SpecialMap[31];

			if( i < 31 && pFont->SpecialMap[i] == 0 )
				pFont->SpecialMap[i] = pFont->SpecialMap[31];
		}
	}	
	
	// Load texture header.
	if( !memory_resident )
	{
		Len = File::Read( FontBuf, 16, 1, p_FH );
		Dbg_MsgAssert( Len == 16, ( "couldn't read texture header from font file %s", Filename ));
	}
	else
	{
		CopyMemory( FontBuf, Filename, 16 );
		Filename += 16;
	}

	Width	= ((uint16 *)FontBuf)[2];
	Height	= ((uint16 *)FontBuf)[3];
	Depth	= ((uint16 *)FontBuf)[4];

	// Create texture.
	Dbg_Assert( Depth == 8 );
	if( D3D_OK != D3DDevice_CreateTexture(	Width, Height, 1, 0, D3DFMT_P8, 0, &pFont->pD3DTexture ))
	{
		Dbg_Assert( 0 );
		return NULL;
	}
	
	// Read texture bitmap data (into temp buffer so we can then swizzle it).
	NumBytes = ( Width * Height + 3 ) & 0xFFFFFFFC;

	uint8* p_temp_texel_data = new uint8[NumBytes];
	if( !memory_resident )
	{
		Len = File::Read( p_temp_texel_data, NumBytes, 1, p_FH );
		Dbg_MsgAssert( Len == NumBytes, ( "Couldn't read texture bitmap from font file %s", Filename ));
	}
	else
	{
		CopyMemory( p_temp_texel_data, Filename, NumBytes );
		Filename += NumBytes;
	}

	// Lock the texture so we can swizzle into it directly.
	D3DLOCKED_RECT locked_rect;
	if( D3D_OK != pFont->pD3DTexture->LockRect( 0, &locked_rect, NULL, 0 ))
	{
		Dbg_Assert( 0 );
		return NULL;
	}
	
	// Swizzle the texture data.
	SwizzleTexture( locked_rect.pBits, p_temp_texel_data, Width, Height, 8, Width );

	// No longer need this data.
	delete[] p_temp_texel_data;	
	
	// Create palette.
	if( D3D_OK != D3DDevice_CreatePalette(	D3DPALETTE_256,	&pFont->pD3DPalette ))
	{
		Dbg_Assert( 0 );
		return NULL;
	}
	
	// Read clut bitmap data.
	D3DCOLOR *p_clut;
	pFont->pD3DPalette->Lock( &p_clut, 0 );

	if( !memory_resident )
	{
		Len	= File::Read( p_clut, 1024, 1, p_FH );
		Dbg_MsgAssert( Len == 1024, ( "couldn't read clut bitmap from font file %s", Filename ));
	}
	else
	{
		CopyMemory( p_clut, Filename, 1024 );
		Filename += 1024;
	}

	// Switch from RGBA to BGRA format palette.
	for( i = 0; i < 256; ++i )
	{
		uint32	red = p_clut[i] & 0xFF;
		uint32	blu = ( p_clut[i] >> 16 ) & 0xFF;

		// Double the alpha in the clut (currently limited to 0x80).
		uint32 alpha = p_clut[i] >> 24;
		alpha = ( alpha >= 0x80 ) ? 0xFF : ( alpha * 2 );
		p_clut[i] = ( alpha << 24 ) | ( p_clut[i] & 0x0000FF00 ) | ( red << 16 ) | ( blu );
	}

	// Skip numsubtextures, and load subtextures.
	if( !memory_resident )
	{
		Len = File::Read( FontBuf, ( NumChars << 3 ) + 4, 1, p_FH );
		Dbg_MsgAssert( Len == ( NumChars << 3 ) + 4, ( "couldn't read subtexture table from font file %s", Filename ));
	}
	else
	{
		CopyMemory( FontBuf, Filename, ( NumChars << 3 ) + 4 );
		Filename += ( NumChars << 3 ) + 4;
	}

	for( i = 0, pChar = pFont->pChars, pData = FontBuf + 4; i < NumChars; i++, pChar++, pData += 8 )
	{
		uint16 x	= ((uint16 *)pData )[0];
		uint16 y	= ((uint16 *)pData )[1];
		uint16 w	= ((uint16 *)pData )[2];
		uint16 h	= ((uint16 *)pData )[3];
		
		pChar->w	= w;
		pChar->h	= h;
		pChar->u0	= (float)x / (float)Width;
		pChar->v0	= (float)y / (float)Height;
		pChar->u1	= pChar->u0 + ((float)w / (float)Width );
		pChar->v1	= pChar->v0 + ((float)h / (float)Height );
	}

	// Add font to font list.
	pFont->pNext	= pFontList;
	pFontList		= pFont;

	// We're done with the font file now.
	if( !memory_resident )
		File::Close( p_FH );
	
	// this will serve as the default spacing
	pFont->mSpaceSpacing = pFont->pChars[pFont->Map['I']].w;
	
	return pFont;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void UnloadFont( SFont *pFont )
{
	SFont*	pPrevFont;
	int		found = 0;

	// Find font and unchain from list.
	if( pFontList == pFont )
	{
		found=1;
		pFontList = pFontList->pNext;
	}
	else
	{
		for( pPrevFont=pFontList; pPrevFont->pNext; pPrevFont=pPrevFont->pNext )
		{
			if( pPrevFont->pNext == pFont )
			{
				found = 1;
				pPrevFont->pNext = pFont->pNext;
				break;
			}
		}
	}

	Dbg_MsgAssert( found, ( "Attempt to unload font which has not been loaded" ));

	// Free memory.
	delete [] pFont->pChars;
	delete pFont;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 SFont::GetDefaultHeight() const
{
	return DefaultHeight;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 SFont::GetDefaultBase() const
{
	return DefaultBase;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SFont::QueryString( char *String, float &width, float &height )
{
	SChar	*pChar;
	char	*pLetter;
	int		x0,x1;

	x0 = 0;

	for (pLetter=String;; pLetter++)
	{
		pChar = NULL;
		// may be overridden by the '\b' tag
		SFont *p_font = this;
		
		// acount for tags (might be multiple ones in a row)
		bool got_char_tag = false; // tag resulting in output of character
		while (*pLetter == '\\' && !got_char_tag)
		{
			pLetter++;
			if (*pLetter == '\\')
				break;

			switch(*pLetter)
			{
				case '\\':
					got_char_tag = true;
					break;
				case 'c':
				case 'C':
					pLetter += 2; // skip over "c#"
					break;
				case 's':
				case 'S':
				{
					pLetter++; // skip "s"
					uint digit = Str::DehexifyDigit(pLetter);
					pChar = pChars + SpecialMap[digit];
					got_char_tag = true;
					break;
				}
				case 'b':
				case 'B':
				{
					pLetter++; // skip "b"
					uint digit = Str::DehexifyDigit(pLetter);
					
					// switch over to buttons font, the regular font will be used again on the next character

					p_font = pButtonsFont;
					Dbg_Assert(p_font);
					pChar = p_font->pChars + p_font->SpecialMap[digit];
					got_char_tag = true;
					break;
				}
				case 'm':
				case 'M':
				{
					pLetter++; // skip "m"
					char button_char = Nx::CFontManager::sMapMetaCharacterToButton(pLetter);
					uint digit = Str::DehexifyDigit(&button_char);
					
					p_font = pButtonsFont;
					Dbg_Assert(p_font);
					pChar = p_font->pChars + p_font->SpecialMap[digit];
					got_char_tag = true;
					break;
				}
				default:
					Dbg_MsgAssert(0, ("unknown tag"));
					break;
			}
		} // end while
		
		if (*pLetter == '\0') break;
		
		if (*pLetter!=' ' || pChar)
		{
			if (!pChar)
				pChar = p_font->pChars + p_font->Map[(uint8)*pLetter];
			x1 = x0 + pChar->w;
		}
		else
		{
			x1 = x0 + mSpaceSpacing;
		}

		//x0 = x1 + mCharSpacing + 1;
		x0 = x1 + mCharSpacing;
	}

	width  = (float)x0;
	height = (float)DefaultHeight;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
SText::SText( float pri ) : SDraw2D( pri, true )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
SText::~SText( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SText::BeginDraw( void )
{
	p_locked_font_vertex_buffer = &( font_vertex_buffer[0] );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SText::Draw( void )
{
	SChar	*pChar;
	char	*pLetter;
	float	u0,v0,x0,y0,u1,v1,x1,y1,yt;

	x0 = SCREEN_CONV_X( m_xpos );
	y0 = SCREEN_CONV_Y( m_ypos );

	float char_spacing	= (float)mp_font->mCharSpacing * m_xscale;
	float space_spacing = (float)mp_font->mSpaceSpacing * m_xscale;
	
	DWORD current_color	= ( m_rgba & 0xFF00FF00 ) | (( m_rgba & 0xFF ) << 16 ) | (( m_rgba & 0xFF0000 ) >> 16 );
	
	float text_z = GetZValue();
	
	for( pLetter = mp_string;; pLetter++ )
	{
		pChar = NULL;
		SFont *p_font = mp_font;

		sFontVert* p_vert = ((sFontVert*)p_locked_font_vertex_buffer ) + font_vertex_offset;

		// acount for tags (might be multiple ones in a row)
		bool got_char_tag = false; // tag resulting in output of character
		while (*pLetter == '\\' && !got_char_tag)
		{
			pLetter++;

			switch(*pLetter)
			{
				case '\\':
					got_char_tag = true;
					break;
				case 'c':
				case 'C':
				{
					pLetter++;	// skip "c"					
					uint digit = Str::DehexifyDigit(pLetter);
					pLetter++; // skip "#"
					
					// Set active color from font.
					if( digit == 0 || m_color_override)
					{
						// Switch from RGBA to BGRA format.
						current_color	= ( m_rgba & 0xFF00FF00 ) | (( m_rgba & 0xFF ) << 16 ) | (( m_rgba & 0xFF0000 ) >> 16 );
					}
					else
					{
						// Switch from RGBA to BGRA format.
						uint32 color	= mp_font->mRGBATab[digit-1];
						current_color	= ( color & 0xFF00FF00 ) | (( color & 0xFF ) << 16 ) | (( color & 0xFF0000 ) >> 16 );
					}
					break;
				}
				case 's':
				case 'S':
				{
					pLetter++;	// skip "s"
					uint digit = Str::DehexifyDigit(pLetter);
					
					pChar = mp_font->pChars + mp_font->SpecialMap[digit];
					got_char_tag = true;
					
					break;
				}
				case 'b':
				case 'B':
				{
					// 'B' stands for button, accesses the button font

					pLetter++; // skip "b"
					uint digit = Str::DehexifyDigit( pLetter );
					
					// switch to the buttons font!
					p_font = pButtonsFont;
					Dbg_Assert( p_font );
					
					pChar = p_font->pChars + p_font->SpecialMap[digit];
					got_char_tag = true;
					
					EndDraw();
					BeginDraw();

					// Reset the vertex data pointer.
					p_vert = ((sFontVert*)p_locked_font_vertex_buffer ) + font_vertex_offset;
					
					spOverrideFont = p_font;
					break;
				}
				default:
				{
					Dbg_MsgAssert( 0, ( "unknown tag" ));
					break;
				}
			}
		} // end while
		
		if (*pLetter == '\0') break;
		
		if( *pLetter != ' ' || pChar)
		{
			if (!pChar)
				pChar = p_font->pChars + p_font->Map[(uint8) *pLetter];
			yt = y0 + ((float)( p_font->DefaultBase - pChar->Baseline ) * m_yscale ) * EngineGlobals.screen_conv_y_multiplier;
			u0 = pChar->u0;
			v0 = pChar->v0;
			u1 = pChar->u1;
			v1 = pChar->v1;
			x1 = x0 + ( pChar->w * m_xscale * EngineGlobals.screen_conv_x_multiplier );
			y1 = yt + ( pChar->h * m_yscale * EngineGlobals.screen_conv_y_multiplier );
		}
		else
		{
			x0 += ( space_spacing + char_spacing ) * EngineGlobals.screen_conv_x_multiplier;
			continue;
		}

		p_vert->x	= x0;
		p_vert->y	= yt;
		p_vert->z	= text_z;
		p_vert->rhw	= 0.0f;
		p_vert->col	= current_color;
		p_vert->u	= u0;
		p_vert->v	= v0;
		++p_vert;

		p_vert->x	= x0;
		p_vert->y	= y1;
		p_vert->z	= text_z;
		p_vert->rhw	= 0.0f;
		p_vert->col	= current_color;
		p_vert->u	= u0;
		p_vert->v	= v1;
		++p_vert;

		p_vert->x	= x1;
		p_vert->y	= y1;
		p_vert->z	= text_z;
		p_vert->rhw	= 0.0f;
		p_vert->col	= current_color;
		p_vert->u	= u1;
		p_vert->v	= v1;
		++p_vert;

		p_vert->x	= x1;
		p_vert->y	= yt;
		p_vert->z	= text_z;
		p_vert->rhw	= 0.0f;
		p_vert->col	= current_color;
		p_vert->u	= u1;
		p_vert->v	= v0;

		font_vertex_offset += 4;

		if( font_vertex_offset >= ( CHARS_PER_BUFFER * 4 ))
		{
			// Draw this buffer and cycle through to the next.
			EndDraw();
			BeginDraw();
		}

		x0 = x1 + ( char_spacing * EngineGlobals.screen_conv_x_multiplier );

		if( p_font != mp_font )
		{
			// We just used the button font, so return to the regular one.
			EndDraw();
			BeginDraw();
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SText::EndDraw( void )
{
	if( font_vertex_offset > 0 )
	{
		// Subsequent processing within Draw() will use this font
		// Draw() may call this function to temporarily switch fonts
		SFont *p_font = ( spOverrideFont ) ? spOverrideFont : mp_font;
		
		// Set up the render state and submit.
		set_pixel_shader( PixelShader4 );
		set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ));

		set_texture( 0, p_font->pD3DTexture, p_font->pD3DPalette );

		EngineGlobals.p_Device->DrawVerticesUP( D3DPT_QUADLIST, font_vertex_offset, &( font_vertex_buffer[0] ), sizeof( sFontVert ));

		// Reset offset.
		font_vertex_offset = 0;

		// We can now return to using the regular font (no override).
		spOverrideFont = NULL;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SetTextWindow( uint16 x0, uint16 x1, uint16 y0, uint16 y1 )
{
}

} // namespace Xbox

