///////////////////////////////////////////////////////////////////////////////
// p_NxTexture.cpp

#include 	"Gfx/gfxutils.h"
#include 	"Gfx/Nx.h"
#include 	"Gfx/NGPS/p_NxTexture.h"
#include 	"Gfx/NGPS/PaletteGen.h"
#include	"Gfx/NGPS/nx/scene.h"
#include	"Gfx/NGPS/nx/sprite.h"
#include	<gel/scripting/checksum.h>
#include	<gel/scripting/script.h>
#include	<gel/scripting/symboltable.h>

#define min(x, y) (((x) > (y))? (y): (x))
#define max(x, y) (((x) < (y))? (y): (x))

#define USE_FAST_COLOR_FIND 1
#define PRINT_TIMES 0
#define PRINT_FAST_TIMES 0

namespace Nx
{

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CTexture

bool				CPs2Texture::s_tables_initialized = false;
uint8				CPs2Texture::s_clut8_index_to_offset_table[256];
uint8				CPs2Texture::s_clut8_offset_to_index_table[256];

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Texture::CPs2Texture(bool loading_screen) :
	m_transparent(false),
	m_loading_screen(loading_screen),
	mp_single_texture(NULL),
	mp_group_texture(NULL),
	mp_orig_temp_32bit_image(NULL),
	mp_cur_temp_32bit_image(NULL),
	mp_temp_32bit_single_texture(NULL)
{
	m_width = m_height = m_bitdepth = m_clut_bitdepth = m_num_mipmaps = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Texture::CPs2Texture(const CPs2Texture & src_texture) :
	CTexture(src_texture)
{
	Dbg_MsgAssert(src_texture.mp_group_texture == NULL, ("Can't copy a CPs2Texture that is in a texture group"));

	m_width = src_texture.m_width;
	m_height = src_texture.m_height;
	m_bitdepth = src_texture.m_bitdepth;
	m_clut_bitdepth = src_texture.m_clut_bitdepth;
	m_num_mipmaps = src_texture.m_num_mipmaps;

	m_transparent = src_texture.m_transparent;
	m_loading_screen = src_texture.m_loading_screen;

	mp_group_texture = NULL;

	if (src_texture.mp_orig_temp_32bit_image)
	{
		mp_orig_temp_32bit_image = new uint32[m_width * m_height];
		memcpy(mp_orig_temp_32bit_image, src_texture.mp_orig_temp_32bit_image, m_width * m_height * sizeof(uint32));
	}
	else
	{
		mp_orig_temp_32bit_image = NULL;
	}

	if (src_texture.mp_cur_temp_32bit_image)
	{
		mp_cur_temp_32bit_image = new uint32[m_width * m_height];
		memcpy(mp_cur_temp_32bit_image, src_texture.mp_cur_temp_32bit_image, m_width * m_height * sizeof(uint32));
	}
	else
	{
		mp_cur_temp_32bit_image = NULL;
	}

	if (src_texture.mp_single_texture)
	{
		mp_single_texture = new NxPs2::SSingleTexture(*src_texture.mp_single_texture);
	}
	else
	{
		mp_single_texture = NULL;
	}

	if (src_texture.mp_temp_32bit_single_texture)
	{
		mp_temp_32bit_single_texture = new NxPs2::SSingleTexture(*src_texture.mp_temp_32bit_single_texture);
	}
	else
	{
		mp_temp_32bit_single_texture = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Texture::~CPs2Texture()
{
	// delete the actual single texture, if any
	if (mp_single_texture)
	{
		delete mp_single_texture;
	}

	if (mp_orig_temp_32bit_image)
	{
		delete mp_orig_temp_32bit_image;
	}

	if (mp_cur_temp_32bit_image)
	{
		delete mp_cur_temp_32bit_image;
	}

	if (mp_temp_32bit_single_texture)
	{
		delete mp_temp_32bit_single_texture;
	}

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	CPs2Texture::sInitTables()
{
    if (!s_tables_initialized)
	{
		uint i, j, idx;

		idx = 0;
        for(i = 0; i < 256; i+=32)
		{
            for(j = i; j < i+8; j++)
                s_clut8_index_to_offset_table[j] = (uint8) idx++;
            for(j = i+16; j < i+16+8; j++)
                s_clut8_index_to_offset_table[j] = (uint8) idx++;
            for(j = i+8;  j < i+8+8;  j++)
                s_clut8_index_to_offset_table[j] = (uint8) idx++;
            for(j = i+24; j < i+24+8; j++)
                s_clut8_index_to_offset_table[j] = (uint8) idx++;
        }

		idx = 0;
		for(i = 0; i < 256; i+=32)
		{
			for(j = i; j < i+8; j++)
				s_clut8_offset_to_index_table[idx++] = (uint8) j;
			for(j = i+16; j < i+16+8; j++)
				s_clut8_offset_to_index_table[idx++] = (uint8) j;
			for(j = i+8;  j < i+8+8;  j++)
				s_clut8_offset_to_index_table[idx++] = (uint8) j;
			for(j = i+24; j < i+24+8; j++)
				s_clut8_offset_to_index_table[idx++] = (uint8) j;
		}
	
        s_tables_initialized = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_load_texture(const char *p_texture_name, bool sprite, bool alloc_vram)
{
	Dbg_Assert(mp_group_texture == NULL);

	// call engine
	mp_single_texture = new NxPs2::SSingleTexture(p_texture_name, sprite, alloc_vram, m_loading_screen);
	if (mp_single_texture)
	{
		m_width			= mp_single_texture->GetOrigWidth();	// Garrett: This is probably not what we want, but CSprite
		m_height		= mp_single_texture->GetOrigHeight();  // needs this.  Probably will need to change CTexture.
		m_bitdepth		= mp_single_texture->GetBitdepth();
		m_clut_bitdepth	= mp_single_texture->GetClutBitdepth();
		m_num_mipmaps	= mp_single_texture->GetNumMipmaps();
		m_transparent	= mp_single_texture->IsTransparent();
	}

	return mp_single_texture != NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_load_texture_from_buffer(uint8* p_buffer, int buffer_size, bool sprite, bool alloc_vram)
{
	Dbg_Assert(mp_group_texture == NULL);

	// call engine
	mp_single_texture = new NxPs2::SSingleTexture(p_buffer, buffer_size, sprite, alloc_vram, m_loading_screen);
	if (mp_single_texture)
	{
		m_width			= mp_single_texture->GetOrigWidth();	// Garrett: This is probably not what we want, but CSprite
		m_height		= mp_single_texture->GetOrigHeight();  // needs this.  Probably will need to change CTexture.
		m_bitdepth		= mp_single_texture->GetBitdepth();
		m_clut_bitdepth	= mp_single_texture->GetClutBitdepth();
		m_num_mipmaps	= mp_single_texture->GetNumMipmaps();
		m_transparent	= mp_single_texture->IsTransparent();
	}

	return mp_single_texture != NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_replace_texture(CTexture *p_texture)
{								 	
	Dbg_Assert(p_texture);
	Dbg_Assert(mp_group_texture);

	CPs2Texture *p_ps2_texture = static_cast<CPs2Texture *>(p_texture);
	Dbg_Assert(p_ps2_texture->mp_single_texture);

	if ((m_width != p_ps2_texture->m_width) ||
		(m_height != p_ps2_texture->m_height) ||
		(m_bitdepth != p_ps2_texture->m_bitdepth) ||
		(m_num_mipmaps != p_ps2_texture->m_num_mipmaps))
	{
		Dbg_Message("Original size (%d, %d); Replacement size (%d, %d)", m_width, m_height, p_ps2_texture->m_width, p_ps2_texture->m_height);
		Dbg_Message("Original bitdepth %d; Replacement bitdepth %d", m_bitdepth, p_ps2_texture->m_bitdepth);
		Dbg_Message("Original num mipmaps %d; Replacement num mipmaps %d", m_num_mipmaps, p_ps2_texture->m_num_mipmaps);
		return false;
	}

#if 0
	if ((m_bitdepth <= 8) && (m_clut_bitdepth < p_ps2_texture->m_clut_bitdepth))
	{
		Dbg_Message("Original clut bitdepth %d; Replacement clut bitdepth %d", m_clut_bitdepth, p_ps2_texture->m_clut_bitdepth);
		return false;
	}
#endif

	// Copy texture
	uint8 *p_orig_tex_buffer = mp_group_texture->GetTextureBuffer();
	uint8 *p_new_tex_buffer = p_ps2_texture->mp_single_texture->GetTextureBuffer();
	if (mp_group_texture->HasSwizzleMip())
	{
		mp_group_texture->ReplaceTextureData(p_new_tex_buffer);
	}
	else
	{
		memcpy(p_orig_tex_buffer, p_new_tex_buffer, p_ps2_texture->mp_single_texture->GetTextureBufferSize());
	}

	// And clut (if any)
	uint32 *p_orig_clut_buffer = (uint32 *) mp_group_texture->GetClutBuffer();
	if (p_orig_clut_buffer)
	{
		uint32 *p_new_clut_buffer = (uint32 *) p_ps2_texture->mp_single_texture->GetClutBuffer();
		Dbg_Assert(p_new_clut_buffer);

		if (m_clut_bitdepth == p_ps2_texture->m_clut_bitdepth)
		{
			memcpy(p_orig_clut_buffer, p_new_clut_buffer, p_ps2_texture->mp_single_texture->GetClutBufferSize());
		} else if ((m_clut_bitdepth == 32) && (p_ps2_texture->m_clut_bitdepth == 16))
		{
			uint16 *p_clut_16_buffer = (uint16 *) p_new_clut_buffer;
			Image::RGBA new_color;
			new_color.a = 0x80;

			int num_colors = p_ps2_texture->mp_single_texture->GetClutBufferSize() / 2;
			for (int i = 0; i < num_colors; i++)
			{
				// Convert from 16 to 32 bit
				new_color.r = ((*(p_clut_16_buffer)   >>  0) & 0x1f) << 3;
				new_color.g = ((*(p_clut_16_buffer)   >>  5) & 0x1f) << 3;
				new_color.b = ((*(p_clut_16_buffer++) >> 10) & 0x1f) << 3;

				//Dbg_Message("Replacing color #%d %x with %x", i, *p_orig_clut_buffer, *((uint32 *) &new_color));
				*(p_orig_clut_buffer++) = *((uint32 *) &new_color);
			}
		} else if ((m_clut_bitdepth == 16) && (p_ps2_texture->m_clut_bitdepth == 32))
		{
			uint16 *p_clut_16_buffer = (uint16 *) p_orig_clut_buffer;
			uint16 new_color;
			Image::RGBA *p_full_color;

			int num_colors = p_ps2_texture->mp_single_texture->GetClutBufferSize() / 4;
			for (int i = 0; i < num_colors; i++)
			{
				p_full_color = (Image::RGBA *) p_new_clut_buffer++;
				new_color = 0x8000 | (p_full_color->r >> 3) | ( (p_full_color->g >> 3) << 5 ) | ( (p_full_color->b >> 3) << 10 );
				*(p_clut_16_buffer++) = new_color;		
			}
		} else {
			Dbg_Message("Can't handle this combination: clut bitdepth %d; Replacement clut bitdepth %d", m_clut_bitdepth, p_ps2_texture->m_clut_bitdepth);
			return false;
		}

		//Dbg_Message("Original clut size %d; Replacement clut size %d", mp_group_texture->GetClutBufferSize(), p_ps2_texture->mp_single_texture->GetClutBufferSize());
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_generate_32bit_image(bool renderable, bool store_original)
{
	Dbg_MsgAssert(!mp_cur_temp_32bit_image, ("Can't generate temp 32-bit image: one already exists"));

	// For now, just coded up the 8-bit image with 32-bit clut
	Dbg_Assert(m_bitdepth == 8);
	Dbg_MsgAssert(m_clut_bitdepth == 32, ("Clut bitdepth is only %d, not 32", m_clut_bitdepth));
	Dbg_Assert(mp_single_texture);
	Dbg_Assert(s_tables_initialized);

	// Allocate buffer
	mp_cur_temp_32bit_image = new uint32[m_width * m_height];
	uint32 *p_pixel_buffer = mp_cur_temp_32bit_image;

	// Get source buffers
	uint8 *p_orig_tex_buffer = mp_single_texture->GetTextureBuffer();
	// And clut (if any)
	uint32 *p_orig_clut_buffer = (uint32 *) mp_single_texture->GetClutBuffer();

	// Move texture buffer to start (and flip y)
	p_orig_tex_buffer += (m_height - 1) * m_width;

	for (uint h = 0; h < m_height; h++)
	{
		for (uint w = 0; w < m_width; w++)
		{
			// Copies from 8-bit to 32-bit
			*p_pixel_buffer++ = p_orig_clut_buffer[ s_clut8_index_to_offset(p_orig_tex_buffer[w]) ];
		}
		p_orig_tex_buffer -= m_width;	// Negative because were flipping the lines
	}

	// Copy to original buffer
	if (store_original)
	{
		mp_orig_temp_32bit_image = new uint32[m_width * m_height];
		memcpy(mp_orig_temp_32bit_image, mp_cur_temp_32bit_image, m_width * m_height * sizeof(uint32));
	}

	// Allocate a SSingleTexture
	if (renderable)
	{
		mp_temp_32bit_single_texture = new NxPs2::SSingleTexture((uint8 *) mp_cur_temp_32bit_image, m_width, m_height, 32,
																 m_clut_bitdepth, m_num_mipmaps, true, true, m_loading_screen);
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_put_32bit_image_into_texture(bool new_palette)
{
	Dbg_MsgAssert(mp_cur_temp_32bit_image, ("No temp 32-bit image"));

	// For now, just coded up the 8-bit image with 32-bit clut
	Dbg_Assert(m_bitdepth == 8);
	Dbg_Assert(m_clut_bitdepth == 32);
	Dbg_Assert(mp_single_texture);
	Dbg_Assert(s_tables_initialized);

	// Generate a new palette
	if (new_palette)
	{
		GeneratePalette((Image::RGBA *) mp_single_texture->GetClutBuffer(), (uint8 *) mp_cur_temp_32bit_image, m_width, m_height, 32, 1 << m_bitdepth);
	}

	// Get buffers
	uint8 *p_tex_buffer = mp_single_texture->GetTextureBuffer();
	uint32 *p_pixel_buffer = mp_cur_temp_32bit_image;

	// Move texture buffer to start (and flip y)
	p_tex_buffer += (m_height - 1) * m_width;

#if USE_FAST_COLOR_FIND
#if PRINT_FAST_TIMES
	uint32 start_time = Tmr::GetTimeInUSeconds();
#endif // PRINT_FAST_TIMES

	setup_fast_clut_color_find(false);

#if PRINT_FAST_TIMES
	uint32 end_time = Tmr::GetTimeInUSeconds();
	Dbg_Message("plat_put_32bit_image_into_texture fast setup time %d us", end_time - start_time);
#endif // PRINT_FAST_TIMES
#endif // USE_FAST_COLOR_FIND

	for (uint h = 0; h < m_height; h++)
	{
		for (uint w = 0; w < m_width; w++)
		{
			// Copies from 32-bit to 8-bit
#if USE_FAST_COLOR_FIND
			p_tex_buffer[w] = s_clut8_offset_to_index( fast_find_closest_clut_color( *((Image::RGBA *) p_pixel_buffer++) ) );
#else
			p_tex_buffer[w] = s_clut8_offset_to_index( find_closest_clut_color( *((Image::RGBA *) p_pixel_buffer++) ) );
#endif // USE_FAST_COLOR_FIND
		}
		p_tex_buffer -= m_width;		// Negative because were flipping the lines
	}

#if USE_FAST_COLOR_FIND
	cleanup_fast_clut_color_find();

#if PRINT_FAST_TIMES
	end_time = Tmr::GetTimeInUSeconds();
	Dbg_Message("plat_put_32bit_image_into_texture fast total time %d us", end_time - start_time);
#endif // PRINT_FAST_TIMES
#endif // USE_FAST_COLOR_FIND

	// Delete buffers
	delete [] mp_cur_temp_32bit_image;
	mp_cur_temp_32bit_image = NULL;

	if (mp_orig_temp_32bit_image)
	{
		delete [] mp_orig_temp_32bit_image;
		mp_orig_temp_32bit_image = NULL;
	}

	if (mp_temp_32bit_single_texture)
	{
		delete mp_temp_32bit_single_texture;
		mp_temp_32bit_single_texture = NULL;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2Texture::s_get_nearest_pixels_from_32bit_texture(Image::RGBA p_nearest[][2], uint32 *p_buffer,
															 uint16 x_pos, uint16 y_pos, uint16 width, uint16 height)
{
	int next_x = (x_pos >= (width - 1)) ? 0 : 1;		// Clamp x if at end
	bool clamp_y = (y_pos >= (height - 1));				// And same for y

	p_buffer += (y_pos * width) + x_pos;

	// First row
	p_nearest[0][0] = *((Image::RGBA *) p_buffer);
	p_nearest[0][1] = *((Image::RGBA *) &p_buffer[next_x]);

	if (!clamp_y)
	{
		p_buffer += width;
	}

	// Second row
	p_nearest[1][0] = *((Image::RGBA *) p_buffer);
	p_nearest[1][1] = *((Image::RGBA *) &p_buffer[next_x]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2Texture::s_get_region(uint8 *p_buffer, uint16 x_pos, uint16 y_pos, uint16 tex_width, uint16 tex_height,
								  uint8 *p_region, uint16 region_width, uint16 region_height, uint16 bitdepth)
{
	int bytes_per_pixel = bitdepth / 8;
	int copy_bytes;
	int width_in_bytes;
	int stride_in_bytes;

	// Clamp region values
	int clamp_region_width = min(region_width, tex_width - x_pos);
	int clamp_region_height = min(region_height, tex_height - y_pos);
	Dbg_Assert(clamp_region_width >= 0);
	Dbg_Assert(clamp_region_height >= 0);

	// Move texture buffer to start
	if (bytes_per_pixel)
	{
		p_buffer += ((y_pos * tex_width) + x_pos) * bytes_per_pixel;
		copy_bytes = clamp_region_width * bytes_per_pixel;
		width_in_bytes = region_width * bytes_per_pixel;
		stride_in_bytes = tex_width * bytes_per_pixel;
	}
	else // 4-bit
	{
		p_buffer += ((y_pos * tex_width) + x_pos) / 2;
		copy_bytes = clamp_region_width / 2;
		width_in_bytes = region_width / 2;
		stride_in_bytes = tex_width / 2;
	}

	//Dbg_Message("s_get_region(): xpos %d, ypos %d, tex_width %d, tex_height %d, region_width %d, region_height %d, width_in_bytes %d, stride_in_bytes %d, bitdepth %d",
	//			x_pos, y_pos, tex_width, tex_height, region_width, region_height, width_in_bytes, stride_in_bytes, bitdepth);
	for (int h = 0; h < clamp_region_height; h++)
	{
		// Copy each line
		memcpy(p_region, p_buffer, copy_bytes);
		p_region += width_in_bytes;
		p_buffer += stride_in_bytes;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2Texture::s_put_region(uint8 *p_buffer, uint16 x_pos, uint16 y_pos, uint16 tex_width, uint16 tex_height,
								  uint8 *p_region, uint16 region_width, uint16 region_height, uint16 bitdepth)
{
	int bytes_per_pixel = bitdepth / 8;
	int copy_bytes;
	int width_in_bytes;
	int stride_in_bytes;

	// Clamp region values
	int clamp_region_width = min(region_width, tex_width - x_pos);
	int clamp_region_height = min(region_height, tex_height - y_pos);
	Dbg_Assert(clamp_region_width >= 0);
	Dbg_Assert(clamp_region_height >= 0);

	// Move texture buffer to start
	if (bytes_per_pixel)
	{
		p_buffer += ((y_pos * tex_width) + x_pos) * bytes_per_pixel;
		copy_bytes = clamp_region_width * bytes_per_pixel;
		width_in_bytes = region_width * bytes_per_pixel;
		stride_in_bytes = tex_width * bytes_per_pixel;
	}
	else // 4-bit
	{
		p_buffer += ((y_pos * tex_width) + x_pos) / 2;
		copy_bytes = clamp_region_width / 2;
		width_in_bytes = region_width / 2;
		stride_in_bytes = tex_width / 2;
	}

	//Dbg_Message("s_put_region(): xpos %d, ypos %d, tex_width %d, tex_height %d, region_width %d, region_height %d, width_in_bytes %d, stride_in_bytes %d, bitdepth %d",
	//			x_pos, y_pos, tex_width, tex_height, region_width, region_height, width_in_bytes, stride_in_bytes, bitdepth);
	for (int h = 0; h < clamp_region_height; h++)
	{
		// Copy each line
		memcpy(p_buffer, p_region, copy_bytes);
		p_region += width_in_bytes;
		p_buffer += stride_in_bytes;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2Texture::s_scale_texture(uint8 *p_buffer, uint16 width, uint16 height,
									 uint8 *p_result_buffer, uint16 new_width, uint16 new_height, uint16 bitdepth)
{
	Dbg_MsgAssert(bitdepth > 8, ("Can't scale a paletted texture.  Convert to unpaletted first."));
	Dbg_Assert(bitdepth == 32);		// We'll worry about 16 and 24 bit later.

#if PRINT_TIMES
	static uint32 total_time = 0;
	uint32 start_time = Tmr::GetTimeInUSeconds();
#endif

	float orig_coord[2] = { 0.0f, 0.0f };
	float delta_dim[2];
	float delta_coord[2] = { 0.0f, 0.0f };
	uint16 orig_index[2] = { 0, 0 };

	Image::RGBA closest_pixels[2][2];
	float pixel_portion[2][2];
	Image::RGBA combined_pixel;

	delta_dim[X] = (float) width / (float) new_width;
	delta_dim[Y] = (float) height / (float) new_height;

	for (int h = 0; h < new_height; h++)
	{
		orig_index[Y] = (uint16) orig_coord[Y];			// Get integer version
		delta_coord[Y] = orig_coord[Y] - orig_index[Y];	// And now get fractional part

		for (int w = 0; w < new_width; w++)
		{
			orig_index[X] = (uint16) orig_coord[X];			// Get integer version
			delta_coord[X] = orig_coord[X] - orig_index[X];	// And now get fractional part

			// Calculate the pixel portions
			pixel_portion[0][0] = (1.0f - delta_coord[X]) * (1.0f - delta_coord[Y]);
			pixel_portion[0][1] =        (delta_coord[X]) * (1.0f - delta_coord[Y]);
			pixel_portion[1][0] = (1.0f - delta_coord[X]) *        (delta_coord[Y]);
			pixel_portion[1][1] =        (delta_coord[X]) *        (delta_coord[Y]);
				
			Dbg_MsgAssert((orig_index[X] + 0) < width, ("X coord out of range: %d", orig_index[X]));
			Dbg_MsgAssert((orig_index[Y] + 0) < height, ("Y coord out of range: %d", orig_index[Y]));

			// Get the 4 nearest pixels
			s_get_nearest_pixels_from_32bit_texture(closest_pixels, (uint32 *) p_buffer, orig_index[X], orig_index[Y], width, height);

			// Calculate each color
			combined_pixel.r = (uint8) ((pixel_portion[0][0] * (float) closest_pixels[0][0].r) +
									    (pixel_portion[0][1] * (float) closest_pixels[0][1].r) +
									    (pixel_portion[1][0] * (float) closest_pixels[1][0].r) +
									    (pixel_portion[1][1] * (float) closest_pixels[1][1].r));
			combined_pixel.g = (uint8) ((pixel_portion[0][0] * (float) closest_pixels[0][0].g) +
									    (pixel_portion[0][1] * (float) closest_pixels[0][1].g) +
									    (pixel_portion[1][0] * (float) closest_pixels[1][0].g) +
									    (pixel_portion[1][1] * (float) closest_pixels[1][1].g));
			combined_pixel.b = (uint8) ((pixel_portion[0][0] * (float) closest_pixels[0][0].b) +
									    (pixel_portion[0][1] * (float) closest_pixels[0][1].b) +
									    (pixel_portion[1][0] * (float) closest_pixels[1][0].b) +
									    (pixel_portion[1][1] * (float) closest_pixels[1][1].b));
			combined_pixel.a = (uint8) ((pixel_portion[0][0] * (float) closest_pixels[0][0].a) +
									    (pixel_portion[0][1] * (float) closest_pixels[0][1].a) +
									    (pixel_portion[1][0] * (float) closest_pixels[1][0].a) +
									    (pixel_portion[1][1] * (float) closest_pixels[1][1].a));

			// And copy
			*((uint32 *) p_result_buffer)++ = *((uint32 *) &(combined_pixel));

			// Advance to next coord
			orig_coord[X] += delta_dim[X];
		}

		orig_coord[X] = 0.0f;
		orig_coord[Y] += delta_dim[Y];
	}

#if PRINT_TIMES
	uint32 end_time = Tmr::GetTimeInUSeconds();
	total_time += end_time - start_time;
	Dbg_Message("scale_texture Update time %d us; Total Time %d", end_time - start_time, total_time);
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2Texture::s_combine_adjacent_borders(uint8 *p_first_rect, uint8 *p_second_rect, uint16 first_width,
												uint16 first_height, uint16 second_stride, float first_portion,
												int split_axis, uint16 bitdepth)
{
	Dbg_MsgAssert(bitdepth > 8, ("Can't adjust the border of a paletted texture.  Convert to unpaletted first."));
	Dbg_Assert(bitdepth == 32);		// We'll worry about 16 and 24 bit later.

	Image::RGBA *p_rect_pixel[2] = { (Image::RGBA *) p_first_rect, (Image::RGBA *) p_second_rect };
	float second_portion = 1.0f - first_portion;

	int num_pixels, inc_pixels;

	// Init
	if (split_axis == X)
	{
		num_pixels = first_width;
		inc_pixels = 1;
	}
	else
	{
		Dbg_Assert(first_width == second_stride);

		num_pixels = first_height;
		inc_pixels = second_stride;
	}

	// Combine the pixels
	for (int pidx = 0; pidx < num_pixels; pidx++)
	{
		p_rect_pixel[1]->r = (uint8) ( (first_portion * (float) p_rect_pixel[0]->r) + (second_portion * (float) p_rect_pixel[1]->r) );
		p_rect_pixel[1]->g = (uint8) ( (first_portion * (float) p_rect_pixel[0]->g) + (second_portion * (float) p_rect_pixel[1]->g) );
		p_rect_pixel[1]->b = (uint8) ( (first_portion * (float) p_rect_pixel[0]->b) + (second_portion * (float) p_rect_pixel[1]->b) );
		p_rect_pixel[1]->a = (uint8) ( (first_portion * (float) p_rect_pixel[0]->a) + (second_portion * (float) p_rect_pixel[1]->a) );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static const int ALPHA_WEIGHT = 2;    // how much more weight to give to alpha
uint8 	CPs2Texture::find_closest_clut_color(Image::RGBA rgba)
{
	// For now, just coded up the 8-bit image with 32-bit clut
	Dbg_Assert(m_bitdepth == 8);
	Dbg_Assert(m_clut_bitdepth == 32);
	Dbg_Assert(mp_single_texture);

	int clutsize = 1 << m_bitdepth;

	Image::RGBA *p_clut_buffer = (Image::RGBA *) mp_single_texture->GetClutBuffer();

	Dbg_Assert(p_clut_buffer);

    int delR, delG, delB, delA;
    int bestDist, curDist;
    uint8 bestIdx = 0;

    bestDist = 0x7FFFFFFF;

    for (int i = 0; i < clutsize; i++)
	{
        delR = (int) (p_clut_buffer)->r   - (int) rgba.r;
        delG = (int) (p_clut_buffer)->g   - (int) rgba.g;
        delB = (int) (p_clut_buffer)->b   - (int) rgba.b;
        delA = (int) (p_clut_buffer++)->a - (int) rgba.a;

        // Calculate 4-dimensional distance between color components
        curDist = (delR * delR) + (delG * delG) + (delB * delB) +
                  (delA * delA * ALPHA_WEIGHT);

        // Choose the index with the smallest distance from src color
        if (curDist < bestDist)
		{
            bestDist = curDist;
            bestIdx = i;
        }
    }

    return bestIdx;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Holds clut index array start and size
struct SClutEntries
{
	uint16	m_clut_buffer_index;
	uint8	m_num_clut_entries;
};

static const int s_axis_level_bits = 3;			// Significant bits used to break down each color into the tree
static const int s_axis_levels = 1 << s_axis_level_bits;
static bool s_use_alpha_axis;
static uint8 *sp_clut_index_buffer = NULL;

//static int s_num_color_searches;
//static int s_num_color_compares;

// Array is of form [R][G][B][A]
static SClutEntries s_palette_octree[s_axis_levels][s_axis_levels][s_axis_levels][s_axis_levels];

void 	CPs2Texture::setup_fast_clut_color_find(bool use_alpha)
{
	// For now, just coded up the 8-bit image with 32-bit clut
	Dbg_Assert(m_bitdepth == 8);
	Dbg_Assert(m_clut_bitdepth == 32);
	Dbg_Assert(mp_single_texture);
	Dbg_Assert(!sp_clut_index_buffer);

	s_use_alpha_axis = use_alpha;

	int clutsize = 1 << m_bitdepth;
	int range = clutsize >> s_axis_level_bits;
	int alpha_axis_levels = (s_use_alpha_axis) ? s_axis_levels : 1;
	int index_buffer_size = s_axis_levels * s_axis_levels * s_axis_levels * alpha_axis_levels * (clutsize / 2);	// Could be made smaller

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	sp_clut_index_buffer = new uint8[index_buffer_size];
	Mem::Manager::sHandle().PopContext();

	int min_level[4];
	int max_level[4];

	const int overlap = (range >> 1);
	const int min_start = -overlap;
	const int max_start = range + overlap;
	const int max_alpha_start = (s_use_alpha_axis) ? max_start : clutsize;

	int buffer_index = 0;

	min_level[0] = min_start;
	max_level[0] = max_start;

	for (int r = 0; r < s_axis_levels; r++)
	{
		min_level[1] = min_start;
		max_level[1] = max_start;

		for (int g = 0; g < s_axis_levels; g++)
		{
			min_level[2] = min_start;
			max_level[2] = max_start;

			for (int b = 0; b < s_axis_levels; b++)
			{
				min_level[3] = min_start;
				max_level[3] = max_alpha_start;

				for (int a = 0; a < alpha_axis_levels; a++)
				{
					SClutEntries *p_palette_node = &s_palette_octree[r][g][b][a];
					p_palette_node->m_clut_buffer_index = buffer_index;
					p_palette_node->m_num_clut_entries = 0;
					Image::RGBA *p_clut_buffer = (Image::RGBA *) mp_single_texture->GetClutBuffer();

					Dbg_MsgAssert(buffer_index < (index_buffer_size - clutsize), ("Running out of index buffer space"));

					for (int index = 0; index < clutsize; index++, p_clut_buffer++)
					{
						if ((p_clut_buffer->r >= min_level[0]) && (p_clut_buffer->r <= max_level[0]) &&
							(p_clut_buffer->g >= min_level[1]) && (p_clut_buffer->g <= max_level[1]) &&
							(p_clut_buffer->b >= min_level[2]) && (p_clut_buffer->b <= max_level[2]) &&
							(p_clut_buffer->a >= min_level[3]) && (p_clut_buffer->a <= max_level[3]))
						{
							p_palette_node->m_num_clut_entries++;
							sp_clut_index_buffer[buffer_index++] = index;
						}

					} // for index

					min_level[3] += range;
					max_level[3] += range;

				} // for a

				min_level[2] += range;
				max_level[2] += range;

			} // for b

			min_level[1] += range;
			max_level[1] += range;

		} // for g

		min_level[0] += range;
		max_level[0] += range;

	} // for r

	//s_num_color_searches = 0;
	//s_num_color_compares = 0;
}


uint8 	CPs2Texture::fast_find_closest_clut_color(Image::RGBA rgba)
{
	// For now, just coded up the 8-bit image with 32-bit clut
	Dbg_Assert(m_bitdepth == 8);
	Dbg_Assert(m_clut_bitdepth == 32);
	Dbg_Assert(mp_single_texture);
	Dbg_Assert(sp_clut_index_buffer);

	int range_bits = m_bitdepth - s_axis_level_bits;

	int r_index = rgba.r >> range_bits;
	int g_index = rgba.g >> range_bits;
	int b_index = rgba.b >> range_bits;
	int a_index = (s_use_alpha_axis) ? rgba.a >> range_bits : 0;

	SClutEntries *p_palette_node = &s_palette_octree[r_index][g_index][b_index][a_index];

	//s_num_color_searches++;
	if (p_palette_node->m_num_clut_entries == 0)
	{
		//s_num_color_compares += 256;
		return find_closest_clut_color(rgba);
	}
	else
	{
		uint8 *p_palette_index = &sp_clut_index_buffer[p_palette_node->m_clut_buffer_index];
		Image::RGBA *p_clut_buffer = (Image::RGBA *) mp_single_texture->GetClutBuffer();
		Dbg_Assert(p_clut_buffer);

		int delR, delG, delB, delA;
		int bestDist, curDist;
		uint8 bestIdx = 0;

		bestDist = 0x7FFFFFFF;

		for (int entry_index = 0; entry_index < p_palette_node->m_num_clut_entries; entry_index++, p_palette_index++)
		{
			Image::RGBA *p_clut_color = &p_clut_buffer[*p_palette_index];

			delR = (int) p_clut_color->r - (int) rgba.r;
			delG = (int) p_clut_color->g - (int) rgba.g;
			delB = (int) p_clut_color->b - (int) rgba.b;
			delA = (int) p_clut_color->a - (int) rgba.a;

			// Calculate 4-dimensional distance between color components
			curDist = (delR * delR) + (delG * delG) + (delB * delB) +
					  (delA * delA * ALPHA_WEIGHT);

			// Choose the index with the smallest distance from src color
			if (curDist < bestDist)
			{
				bestDist = curDist;
				bestIdx = *p_palette_index;
			}
		}

		//s_num_color_compares += p_palette_node->m_num_clut_entries;

		return bestIdx;
	}
}

void 	CPs2Texture::cleanup_fast_clut_color_find()
{
	Dbg_Assert(sp_clut_index_buffer);

	//Dbg_Message("Average color compares per search: %f", (float) s_num_color_compares / (float) s_num_color_searches);

	delete [] sp_clut_index_buffer;
	sp_clut_index_buffer = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2Texture::copy_region_to_32bit_buffer(uint32 *p_pixel_buffer, uint16 x_pos, uint16 y_pos,
												 uint16 width, uint16 height)
{
	// For now, just coded up the 8-bit image with 32-bit clut
	Dbg_Assert(m_bitdepth == 8);
	Dbg_MsgAssert(m_clut_bitdepth == 32, ("Clut bitdepth is only %d, not 32", m_clut_bitdepth));
	Dbg_Assert(mp_single_texture);
	Dbg_Assert(s_tables_initialized);

#if PRINT_TIMES
	static uint32 total_time = 0;
	uint32 start_time = Tmr::GetTimeInUSeconds();
#endif

	if (mp_cur_temp_32bit_image)
	{
		// Get source buffer
		uint32 *p_orig_tex_buffer = mp_cur_temp_32bit_image;

		// Move texture buffer to start
		p_orig_tex_buffer += (y_pos * m_width) + x_pos;

		for (uint h = 0; h < height; h++)
		{
			memcpy(p_pixel_buffer, p_orig_tex_buffer, width * sizeof(uint32));
			p_pixel_buffer += width;
			p_orig_tex_buffer += m_width;
		}
	}
	else
	{
		// Get source buffers
		uint8 *p_orig_tex_buffer = mp_single_texture->GetTextureBuffer();
		// And clut (if any)
		uint32 *p_orig_clut_buffer = (uint32 *) mp_single_texture->GetClutBuffer();

		// Move texture buffer to start (and flip y)
		p_orig_tex_buffer += ((m_height - 1 - y_pos) * m_width) + x_pos;

		for (uint h = 0; h < height; h++)
		{
			for (uint w = 0; w < width; w++)
			{
				// Copies from 8-bit to 32-bit
				*p_pixel_buffer++ = p_orig_clut_buffer[ s_clut8_index_to_offset(p_orig_tex_buffer[w]) ];
			}
			p_orig_tex_buffer -= m_width;	// Negative because were flipping the lines
		}
	}

#if PRINT_TIMES
	uint32 end_time = Tmr::GetTimeInUSeconds();
	total_time += end_time - start_time;
	Dbg_Message("copy_region_to_32bit_buffer Update time %d us; Total Time %d", end_time - start_time, total_time);
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2Texture::blit_32bit_buffer_to_texture(uint32 *p_pixel_buffer, uint16 x_pos, uint16 y_pos,
												  uint16 width, uint16 height)
{
	// For now, just coded up the 8-bit image with 32-bit clut
	Dbg_Assert(m_bitdepth == 8);
	Dbg_Assert(m_clut_bitdepth == 32);
	Dbg_Assert(mp_single_texture);
	Dbg_Assert(s_tables_initialized);

#if PRINT_TIMES
	static uint32 total_time = 0;
	uint32 start_time = Tmr::GetTimeInUSeconds();
#endif

	if (mp_cur_temp_32bit_image)
	{
		// Get source buffer
		uint32 *p_tex_buffer = mp_cur_temp_32bit_image;

		// Move texture buffer to start
		p_tex_buffer += (y_pos * m_width) + x_pos;

		for (uint h = 0; h < height; h++)
		{
			memcpy(p_tex_buffer, p_pixel_buffer, width * sizeof(uint32));
			p_pixel_buffer += width;
			p_tex_buffer += m_width;
		}
	}
	else
	{
		// Get source buffers
		uint8 *p_tex_buffer = mp_single_texture->GetTextureBuffer();

		// Move texture buffer to start (and flip y)
		p_tex_buffer += ((m_height - 1 - y_pos) * m_width) + x_pos;

		for (uint h = 0; h < height; h++)
		{
			for (uint w = 0; w < width; w++)
			{
				// Copies from 32-bit to 8-bit
				p_tex_buffer[w] = s_clut8_offset_to_index( find_closest_clut_color( *((Image::RGBA *) p_pixel_buffer++) ) );
			}
			p_tex_buffer -= m_width;		// Negative because were flipping the lines
		}
	}

#if PRINT_TIMES
	uint32 end_time = Tmr::GetTimeInUSeconds();
	total_time += end_time - start_time;
	Dbg_Message("blit_32bit_buffer_to_texture Update time %d us; Total Time %d", end_time - start_time, total_time);
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2Texture::set_32bit_pixel_to_texture(uint32 pixel, uint16 x_pos, uint16 y_pos)
{
	// For now, just coded up the 8-bit image with 32-bit clut
	Dbg_Assert(m_bitdepth == 8);
	Dbg_Assert(m_clut_bitdepth == 32);
	Dbg_Assert(mp_single_texture);
	Dbg_Assert(s_tables_initialized);

	if (mp_cur_temp_32bit_image)
	{
		// Set pixel in buffer
		mp_cur_temp_32bit_image[(y_pos * m_width) + x_pos] = pixel;
	}
	else
	{
		// Get source buffers
		uint8 *p_tex_buffer = mp_single_texture->GetTextureBuffer();

		// Move texture buffer to start (and flip y)
		p_tex_buffer[((m_height - 1 - y_pos) * m_width) + x_pos] = s_clut8_offset_to_index( find_closest_clut_color( *((Image::RGBA *) &pixel) ) );
	}

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_offset(int x_pixels, int y_pixels, bool use_fill_color, Image::RGBA fill_color)
{
	int region_start_pos[2];
	int region_end_pos[2];
	int region_width;
	int region_height;

	// Figure out X info
	if (x_pixels >= 0)
	{
		region_start_pos[X] = 0;
		region_end_pos[X] = x_pixels;
		region_width = m_width - x_pixels;
	}
	else
	{
		region_start_pos[X] = -x_pixels;
		region_end_pos[X] = 0;
		region_width = m_width - (-x_pixels);
	}

	// Figure out Y info
	if (y_pixels >= 0)
	{
		region_start_pos[Y] = 0;
		region_end_pos[Y] = y_pixels;
		region_height = m_height - y_pixels;
	}
	else
	{
		region_start_pos[Y] = -y_pixels;
		region_end_pos[Y] = 0;
		region_height = m_height - (-y_pixels);
	}

	// Copy texture region into temp buffers in 32-bit format
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	uint32 *p_texture_buffer = new uint32[region_width * region_height];
	Mem::Manager::sHandle().PopContext();

	// Get section that will be in offset texture
	copy_region_to_32bit_buffer(p_texture_buffer, region_start_pos[X], region_start_pos[Y], region_width, region_height);

	// And put that region back into texture
	blit_32bit_buffer_to_texture(p_texture_buffer, region_end_pos[X], region_end_pos[Y], region_width, region_height);

	//static uint32 total_time = 0;
	//uint32 start_time = Tmr::GetTimeInUSeconds();

	// Check if we have a fill color
	if (use_fill_color)
	{
		Dbg_MsgAssert(0, ("Fill color not implemented yet"));
	}
	else
	{
		for (int h = 0, y_pos = -region_end_pos[Y]; h < m_height; h++, y_pos++)
		{
			int y_clamp_pos = max(y_pos, 0);
			y_clamp_pos = min(y_clamp_pos, region_height - 1);
			int y_buffer_index = y_clamp_pos * region_width;

			for (int w = 0, x_pos = -region_end_pos[X]; w < m_width; w++, x_pos++)
			{
				// Check if we are within the new region
				if ((x_pos >= 0) && (x_pos < region_width) &&
					(y_pos >= 0) && (y_pos < region_height))
				{
					continue;
				}

				int x_clamp_pos = max(x_pos, 0);
				x_clamp_pos = min(x_clamp_pos, region_width - 1);

				set_32bit_pixel_to_texture(p_texture_buffer[y_buffer_index + x_clamp_pos], w, h);
			}
		}
	}

	//uint32 end_time = Tmr::GetTimeInUSeconds();
	//total_time += end_time - start_time;
	//Dbg_Message("offset_texture fill Update time %d us; Total Time %d", end_time - start_time, total_time);

	delete [] p_texture_buffer;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_adjust_region(uint16 x_pos, uint16 y_pos, uint16 width, uint16 height,
										int split_axis, uint16 start_point, uint16 end_point)
{								 	
	// Copy original rectangle in a temp buffer in 32-bit format (needed for interpolation)
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	uint32 *p_orig_region_pixels = new uint32[width * height];
	Mem::Manager::sHandle().PopContext();

	copy_region_to_32bit_buffer(p_orig_region_pixels, x_pos, y_pos, width, height);

	float first_rect_portion;
	uint16 rect_pos[2][2];
	uint16 rect_new_pos[2];
	uint16 rect_orig_width[2];
	uint16 rect_orig_height[2];
	uint16 rect_new_width[2];
	uint16 rect_new_height[2];

	// Figure out first rectangle
	rect_pos[0][X] = 0;
	rect_pos[0][Y] = 0;

	if (split_axis == X)
	{
		Dbg_Assert(start_point >= x_pos);
		Dbg_Assert(start_point <= x_pos + width);
		Dbg_Assert(end_point >= x_pos);
		Dbg_Assert(end_point <= x_pos + width);

		// Dimensions of first rectangle
		rect_orig_width[0] = start_point - x_pos + 1;
		rect_orig_height[0] = height;
		rect_new_width[0] = end_point - x_pos + 1;
		rect_new_height[0] = height;
		
		// Second rectangle
		rect_pos[1][X] = start_point - x_pos;
		rect_pos[1][Y] = 0;
		rect_new_pos[X] = end_point - x_pos;
		rect_new_pos[Y] = 0;

		rect_orig_width[1] = width - (start_point - x_pos);
		rect_orig_height[1] = height;
		rect_new_width[1] = width - (end_point - x_pos);
		rect_new_height[1] = height;

		// Figure out portion of first rectangle to whole region
		first_rect_portion = (float) rect_new_width[0] / (float) width;
	}
	else
	{
		Dbg_Assert(start_point >= y_pos);
		Dbg_Assert(start_point <= y_pos + height);
		Dbg_Assert(end_point >= y_pos);
		Dbg_Assert(end_point <= y_pos + height);

		// Dimensions of first rectangle
		rect_orig_width[0] = width;
		rect_orig_height[0] = start_point - y_pos + 1;
		rect_new_width[0] = width;
		rect_new_height[0] = end_point - y_pos + 1;
		
		// Second rectangle
		rect_pos[1][X] = 0;
		rect_pos[1][Y] = start_point - y_pos;
		rect_new_pos[X] = 0;
		rect_new_pos[Y] = end_point - y_pos;

		rect_orig_width[1] = width;
		rect_orig_height[1] = height - (start_point - y_pos);
		rect_new_width[1] = width;
		rect_new_height[1] = height - (end_point - y_pos);

		// Figure out portion of first rectangle to whole region
		first_rect_portion = (float) rect_new_height[0] / (float) height;
	}

	// Make room for the original and new rectangles
	uint32 *p_orig_rect_pixels[2];
	uint32 *p_new_rect_pixels[2];

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	p_orig_rect_pixels[0] = new uint32[rect_orig_width[0] * rect_orig_height[0]];
	p_orig_rect_pixels[1] = new uint32[rect_orig_width[1] * rect_orig_height[1]];

	p_new_rect_pixels[0] = new uint32[rect_new_width[0] * rect_new_height[0]];
	p_new_rect_pixels[1] = new uint32[rect_new_width[1] * rect_new_height[1]];
	Mem::Manager::sHandle().PopContext();

	// Get the original rectangles
	s_get_region((uint8 *) p_orig_region_pixels, rect_pos[0][X], rect_pos[0][Y], width, height,
				 (uint8 *) p_orig_rect_pixels[0], rect_orig_width[0], rect_orig_height[0], 32);
	s_get_region((uint8 *) p_orig_region_pixels, rect_pos[1][X], rect_pos[1][Y], width, height,
				 (uint8 *) p_orig_rect_pixels[1], rect_orig_width[1], rect_orig_height[1], 32);

	// Scale each rectangle
	s_scale_texture((uint8 *) p_orig_rect_pixels[0], rect_orig_width[0], rect_orig_height[0],
					(uint8 *) p_new_rect_pixels[0], rect_new_width[0], rect_new_height[0], 32);
	s_scale_texture((uint8 *) p_orig_rect_pixels[1], rect_orig_width[1], rect_orig_height[1],
					(uint8 *) p_new_rect_pixels[1], rect_new_width[1], rect_new_height[1], 32);

	// Combine two adjacent side lines into 1 (and put into the second rectangle since it will overwrite the first)
	s_combine_adjacent_borders((uint8 *) p_new_rect_pixels[0], (uint8 *) p_new_rect_pixels[1],
										 rect_new_width[0], rect_new_height[0], rect_new_width[1], first_rect_portion, split_axis, 32);

	// Put new rectangles back into region
	s_put_region((uint8 *) p_orig_region_pixels, rect_pos[0][X], rect_pos[0][Y], width, height,
				 (uint8 *) p_new_rect_pixels[0], rect_new_width[0], rect_new_height[0], 32);
	s_put_region((uint8 *) p_orig_region_pixels, rect_new_pos[X], rect_new_pos[Y], width, height,
				 (uint8 *) p_new_rect_pixels[1], rect_new_width[1], rect_new_height[1], 32);

	// And put region back into texture
	blit_32bit_buffer_to_texture(p_orig_region_pixels, x_pos, y_pos, width, height);

	// Free all the buffers
	delete [] p_orig_rect_pixels[0];
	delete [] p_orig_rect_pixels[1];

	delete [] p_new_rect_pixels[0];
	delete [] p_new_rect_pixels[1];

	delete [] p_orig_region_pixels;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_pull_to_edge(uint16 point, int axis, int num_pixels)
{
#if PRINT_TIMES
	static uint32 total_time = 0;
	uint32 start_time = Tmr::GetTimeInUSeconds();
#endif

	uint32 *p_texture_buffer = NULL;
	uint32 *p_scaled_texture_buffer = NULL;

	uint16 rect_pos[2];
	uint16 rect_orig_width;
	uint16 rect_orig_height;
	uint16 rect_new_width;
	uint16 rect_new_height;
	uint16 scaled_offset;

	if (num_pixels == 0)
	{
		// Nothing to do
		return true;
	}

	// Cap num_pixels on low end
	if ((point + num_pixels) <= 0)
	{
		num_pixels = -point + 1;
	}

	Dbg_Assert((point + num_pixels) >= 0);

	if (axis == X)
	{
		Dbg_Assert(point < m_width);

		// Cap num_pixels on high end
		if ((point + num_pixels) >= m_width)
		{
			num_pixels = m_width - point - 1;
		}

		Dbg_Assert((point + num_pixels) < m_width);

		bool left = (num_pixels < 0);

		if (left)
		{
			// Figure out orig rectangle
			rect_pos[X] = 0;
			rect_pos[Y] = 0;
			rect_orig_width = point + 1;
			rect_orig_height = m_height;

			// Figure out new rectangle
			rect_new_width = point + (-num_pixels) + 1;
			rect_new_height = m_height;

			scaled_offset = -num_pixels;
		}
		else
		{
			// Figure out orig rectangle
			rect_pos[X] = point;
			rect_pos[Y] = 0;
			rect_orig_width = (m_width - point);
			rect_orig_height = m_height;

			// Figure out new rectangle
			rect_new_width = (m_width - point) + num_pixels;
			rect_new_height = m_height;

			scaled_offset = 0;
		}
	}
	else
	{
		Dbg_Assert(point < m_height);

		// Cap num_pixels on high end
		if ((point + num_pixels) >= m_height)
		{
			num_pixels = m_height - point - 1;
		}

		Dbg_Assert((point + num_pixels) < m_height);

		bool top = (num_pixels < 0);

		if (top)
		{
			// Figure out orig rectangle
			rect_pos[X] = 0;
			rect_pos[Y] = 0;
			rect_orig_width = m_width;
			rect_orig_height = point + 1;

			// Figure out new rectangle
			rect_new_width = m_width;
			rect_new_height = point + (-num_pixels) + 1;

			scaled_offset = -num_pixels * m_width;
		}
		else
		{
			// Figure out orig rectangle
			rect_pos[X] = 0;
			rect_pos[Y] = point;
			rect_orig_width = m_width;
			rect_orig_height = (m_height - point);

			// Figure out new rectangle
			rect_new_width = m_width;
			rect_new_height = (m_height - point) + num_pixels;

			scaled_offset = 0;
		}
	}

	// Copy texture region into temp buffers in 32-bit format
	if (Mem::Manager::sHandle().CutsceneTopDownHeap())
	{
		// GJ FIX 9/9/03 FOR SK5:TT13114 - "Failing to allocate 
		// face texture memory in cutscenes"
		// if the cutscene top down heap exists, we want to use
		// that first, because there won't be much on the
		// top down heap during cutscenes...
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneTopDownHeap());
	}
	else
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	}
	p_texture_buffer = new uint32[rect_orig_width * rect_orig_height];
	p_scaled_texture_buffer = new uint32[rect_new_width * rect_new_height];
	Mem::Manager::sHandle().PopContext();

	//Dbg_Message("Copying region from (%d, %d) of size (%d, %d)", rect_pos[X], rect_pos[Y], rect_orig_width, rect_orig_height);
	copy_region_to_32bit_buffer(p_texture_buffer, rect_pos[X], rect_pos[Y], rect_orig_width, rect_orig_height);

	// Scale the rectangle
	s_scale_texture((uint8 *) p_texture_buffer, rect_orig_width, rect_orig_height,
					(uint8 *) p_scaled_texture_buffer, rect_new_width, rect_new_height, 32);

	//Dbg_Message("Pull: Original size (%d, %d), new size (%d, %d)", rect_orig_width, rect_orig_height, rect_new_width, rect_new_height);
	// Crop the rectangle and put back into original buffer
	s_put_region((uint8 *) p_texture_buffer, 0, 0, rect_orig_width, rect_orig_height,
				 (uint8 *) (p_scaled_texture_buffer + scaled_offset), rect_new_width, rect_new_height, 32);

	// And put region back into texture
	blit_32bit_buffer_to_texture(p_texture_buffer, rect_pos[X], rect_pos[Y], rect_orig_width, rect_orig_height);

	// Free buffers
	if (p_texture_buffer)
	{
		delete [] p_texture_buffer;
	}
	if (p_scaled_texture_buffer)
	{
		delete [] p_scaled_texture_buffer;
	}

#if PRINT_TIMES
	uint32 end_time = Tmr::GetTimeInUSeconds();
	total_time += end_time - start_time;
	Dbg_Message("pull_to_edge Update time %d us; Total Time %d", end_time - start_time, total_time);
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_push_to_point(uint16 point, int axis, int num_pixels, bool use_fill_color, Image::RGBA fill_color)
{
#if PRINT_TIMES
	static uint32 total_time = 0;
	uint32 start_time = Tmr::GetTimeInUSeconds();
#endif

	uint32 *p_texture_buffer = NULL;
	uint32 *p_scaled_texture_buffer = NULL;

	uint16 rect_pos[2];
	uint16 rect_new_offset[2];
	uint16 rect_orig_width;
	uint16 rect_orig_height;
	uint16 rect_new_width;
	uint16 rect_new_height;

	if (num_pixels == 0)
	{
		// Nothing to do
		return true;
	}
	//Dbg_Message("Moving %d pixels along %d axis starting at %d", num_pixels, axis, point);

	// Cap num_pixels on low end
	if ((point + num_pixels) <= 0)
	{
		num_pixels = -point + 1;
	}

	Dbg_Assert((point + num_pixels) >= 0);

	if (axis == X)
	{
		Dbg_Assert(point < m_width);

		// Cap num_pixels on high end
		if ((point + num_pixels) >= m_width)
		{
			num_pixels = m_width - point - 1;
		}

		Dbg_Assert((point + num_pixels) < m_width);

		bool left = (num_pixels < 0);

		if (left)
		{
			// Figure out orig rectangle
			rect_pos[X] = point;
			rect_pos[Y] = 0;
			rect_orig_width = (m_width - point);
			rect_orig_height = m_height;

			// Figure out new rectangle
			rect_new_offset[X] = 0;
			rect_new_offset[Y] = 0;
			rect_new_width = (m_width - point) - (-num_pixels);
			rect_new_height = m_height;
		}
		else
		{
			// Figure out orig rectangle
			rect_pos[X] = 0;
			rect_pos[Y] = 0;
			rect_orig_width = point + 1;
			rect_orig_height = m_height;

			// Figure out new rectangle
			rect_new_offset[X] = num_pixels;
			rect_new_offset[Y] = 0;
			rect_new_width = point - num_pixels + 1;
			rect_new_height = m_height;
		}
	}
	else
	{
		Dbg_Assert(point < m_height);

		// Cap num_pixels on high end
		if ((point + num_pixels) >= m_height)
		{
			num_pixels = m_height - point - 1;
		}

		Dbg_Assert((point + num_pixels) < m_height);

		bool top = (num_pixels < 0);

		if (top)
		{
			// Figure out orig rectangle
			rect_pos[X] = 0;
			rect_pos[Y] = point;
			rect_orig_width = m_width;
			rect_orig_height = (m_height - point);

			// Figure out new rectangle
			rect_new_offset[X] = 0;
			rect_new_offset[Y] = 0;
			rect_new_width = m_width;
			rect_new_height = (m_height - point) - (-num_pixels);
		}
		else
		{
			// Figure out orig rectangle
			rect_pos[X] = 0;
			rect_pos[Y] = 0;
			rect_orig_width = m_width;
			rect_orig_height = point + 1;

			// Figure out new rectangle
			rect_new_offset[X] = 0;
			rect_new_offset[Y] = num_pixels;
			rect_new_width = m_width;
			rect_new_height = point - num_pixels + 1;
		}
	}

	Dbg_Assert(rect_new_width);
	Dbg_Assert(rect_new_height);

	// Copy texture region into temp buffers in 32-bit format
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	p_texture_buffer = new uint32[rect_orig_width * rect_orig_height];
	p_scaled_texture_buffer = new uint32[rect_new_width * rect_new_height];
	Mem::Manager::sHandle().PopContext();

	//Dbg_Message("Copying region from (%d, %d) of size (%d, %d)", rect_pos[X], rect_pos[Y], rect_orig_width, rect_orig_height);
	copy_region_to_32bit_buffer(p_texture_buffer, rect_pos[X], rect_pos[Y], rect_orig_width, rect_orig_height);

	// Scale the rectangle
	s_scale_texture((uint8 *) p_texture_buffer, rect_orig_width, rect_orig_height,
					(uint8 *) p_scaled_texture_buffer, rect_new_width, rect_new_height, 32);

	// Check if we have a fill color
	if (use_fill_color)
	{
		int num_pixels = rect_orig_width * rect_orig_height;

		// Just brute force it for now
		// Garrett: This doesn't work since p_texture_buffer is only a subset of the texture
		Dbg_MsgAssert(0, ("Fill color not implemented properly yet"));
		for (int i = 0; i < num_pixels; i++)
		{
			p_texture_buffer[i] = *((uint32 *) &fill_color);
		}
	}

	// Put smaller rectangle back into original buffer
	s_put_region((uint8 *) p_texture_buffer, rect_new_offset[X], rect_new_offset[Y], rect_orig_width, rect_orig_height,
				 (uint8 *) p_scaled_texture_buffer, rect_new_width, rect_new_height, 32);

	// And put region back into texture
	blit_32bit_buffer_to_texture(p_texture_buffer, rect_pos[X], rect_pos[Y], rect_orig_width, rect_orig_height);

	// Cover up old pixels
	if (!use_fill_color)
	{
		if (axis == X)
		{
			bool use_left = (num_pixels > 0);
			int scaled_coord = (use_left) ? 0 : rect_new_width - 1; 

			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
			uint32 *p_strip_buffer = new uint32[m_height];
			Mem::Manager::sHandle().PopContext();
			for (int pidx = 0; pidx < m_height; pidx++)
			{
				p_strip_buffer[pidx] = p_scaled_texture_buffer[(pidx * rect_new_width) + scaled_coord];
			}

			int start = (use_left) ? 0 : m_width - (-num_pixels);
			int end = (use_left) ? num_pixels - 1 : m_width - 1;

			for (int line = start; line <= end; line++)
			{
				blit_32bit_buffer_to_texture(p_strip_buffer, line, 0, 1, m_height);
			}

			delete [] p_strip_buffer;
		}
		else
		{
			bool use_top = (num_pixels > 0);
			int scaled_coord = (use_top) ? 0 : rect_new_height - 1; 

			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
			uint32 *p_strip_buffer = new uint32[m_width];
			Mem::Manager::sHandle().PopContext();
			memcpy(p_strip_buffer, &(p_scaled_texture_buffer[scaled_coord * rect_new_width]), m_width * sizeof(uint32));

			int start = (use_top) ? 0 : m_height - (-num_pixels);
			int end = (use_top) ? num_pixels - 1 : m_height - 1;

			for (int line = start; line <= end; line++)
			{
				blit_32bit_buffer_to_texture(p_strip_buffer, 0, line, m_width, 1);
			}

			delete [] p_strip_buffer;
		}
	}

	// Free buffers
	if (p_texture_buffer)
	{
		delete [] p_texture_buffer;
	}
	if (p_scaled_texture_buffer)
	{
		delete [] p_scaled_texture_buffer;
	}

#if PRINT_TIMES
	uint32 end_time = Tmr::GetTimeInUSeconds();
	total_time += end_time - start_time;
	Dbg_Message("push_to_point Update time %d us; Total Time %d", end_time - start_time, total_time);
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_adjust_brightness(float brightness_scale, bool force_adjust_current)
{
#if PRINT_TIMES
	static uint32 total_time = 0;
	uint32 start_time = Tmr::GetTimeInUSeconds();
#endif

	Dbg_Assert(mp_single_texture);
	Dbg_MsgAssert(mp_cur_temp_32bit_image, ("For now, AdjustBrightness() only supports 32-bit mode"));

	Image::RGBA *p_src_pixels;
	Image::RGBA *p_dst_pixels = (Image::RGBA *) mp_cur_temp_32bit_image;

	// Figure out the source
	if (mp_orig_temp_32bit_image && !force_adjust_current)
	{
		p_src_pixels = (Image::RGBA *) mp_orig_temp_32bit_image;
	}
	else
	{
		p_src_pixels = (Image::RGBA *) mp_cur_temp_32bit_image;
	}

	// Adjust each pixel
	int num_pixels = m_width * m_height;
	for (int i = 0; i < num_pixels; i++, p_src_pixels++, p_dst_pixels++)
	{
		p_dst_pixels->r = (uint8) min(p_src_pixels->r * brightness_scale, 255);
		p_dst_pixels->g = (uint8) min(p_src_pixels->g * brightness_scale, 255);
		p_dst_pixels->b = (uint8) min(p_src_pixels->b * brightness_scale, 255);
	}

#if PRINT_TIMES
	uint32 end_time = Tmr::GetTimeInUSeconds();
	total_time += end_time - start_time;
	Dbg_Message("adjust_brightness(%f) Update time %d us; Total Time %d", brightness_scale, end_time - start_time, total_time);
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_adjust_hsv(float h, float s, float v, bool force_adjust_current)
{
#if PRINT_TIMES
	static uint32 total_time = 0;
	uint32 start_time = Tmr::GetTimeInUSeconds();
#endif

	Dbg_Assert(mp_single_texture);
	Dbg_MsgAssert(mp_cur_temp_32bit_image, ("For now, AdjustHSV() only supports 32-bit mode"));

	Dbg_MsgAssert((h >= 0.0f) && (h <= 360.0f), ("h is out of range: %f", h));
	Dbg_MsgAssert(s >= 0.0f, ("s is negative: %f", s));
	Dbg_MsgAssert(v >= 0.0f, ("v is negative: %f", v));

	Image::RGBA *p_src_pixels;
	Image::RGBA *p_dst_pixels = (Image::RGBA *) mp_cur_temp_32bit_image;

	// Figure out the source
	if (mp_orig_temp_32bit_image && !force_adjust_current)
	{
		p_src_pixels = (Image::RGBA *) mp_orig_temp_32bit_image;
	}
	else
	{
		p_src_pixels = (Image::RGBA *) mp_cur_temp_32bit_image;
	}

	// Adjust each pixel
	float pixel_h, pixel_s, pixel_v;
	float pixel_r, pixel_g, pixel_b;
	int num_pixels = m_width * m_height;
	for (int i = 0; i < num_pixels; i++, p_src_pixels++, p_dst_pixels++)
	{
		// Convert
		Gfx::inlineRGBtoHSV(p_src_pixels->r / 255.0f, p_src_pixels->g / 255.0f, p_src_pixels->b / 255.0f, pixel_h, pixel_s, pixel_v);

		// Adjust
		pixel_h += h;
		if (pixel_h > 360.0f)
		{
			pixel_h -= 360.0f;
		}
		pixel_s = Mth::Min(pixel_s * s, 1.0f);
		pixel_v = Mth::Min(pixel_v * v, 1.0f);

		// Convert back
		Gfx::inlineHSVtoRGB(pixel_r, pixel_g, pixel_b, pixel_h, pixel_s, pixel_v);
		p_dst_pixels->r = (unsigned char)( pixel_r * 255.0f + 0.5f );
		p_dst_pixels->g = (unsigned char)( pixel_g * 255.0f + 0.5f );
		p_dst_pixels->b = (unsigned char)( pixel_b * 255.0f + 0.5f );
	}

#if PRINT_TIMES
	uint32 end_time = Tmr::GetTimeInUSeconds();
	total_time += end_time - start_time;
	Dbg_Message("adjust_hsv(%f) Update time %d us; Total Time %d", s, end_time - start_time, total_time);
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_add_to_vram()
{								 	
	Dbg_MsgAssert(mp_single_texture, ("CPs2Texture::plat_add_to_vram() only works for sSingleTexture types"));

	return mp_single_texture->AddToVRAM();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_remove_from_vram()
{								 	
	Dbg_MsgAssert(mp_single_texture, ("CPs2Texture::plat_remove_from_vram() only works for sSingleTexture types"));

	return mp_single_texture->RemoveFromVRAM();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint16	CPs2Texture::plat_get_width() const
{
	return m_width;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint16	CPs2Texture::plat_get_height() const
{
	return m_height;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CPs2Texture::plat_get_bitdepth() const
{
	return m_bitdepth;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CPs2Texture::plat_get_palette_bitdepth() const
{
	return m_clut_bitdepth;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CPs2Texture::plat_get_num_mipmaps() const
{
	return m_num_mipmaps;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_is_transparent() const
{
	return m_transparent;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CPs2Texture::plat_combine_textures(CTexture *p_texture, bool palette_gen)
{
	CPs2Texture *p_ps2_texture = static_cast<CPs2Texture *>(p_texture);

	// Make sure both textures have same attributes
	Dbg_Assert(m_width == p_ps2_texture->m_width);
	Dbg_Assert(m_height == p_ps2_texture->m_height);
	Dbg_Assert(m_bitdepth == p_ps2_texture->m_bitdepth);
	Dbg_Assert(m_clut_bitdepth == p_ps2_texture->m_clut_bitdepth);

	Dbg_Assert((m_bitdepth == 8) || (m_bitdepth == 32));		// Maybe support more later
	Dbg_Assert(mp_single_texture);
	Dbg_Assert(p_ps2_texture->mp_single_texture);

	bool paletted = (m_bitdepth <= 8);
	uint32 *p_texture1_buffer = NULL;
	uint32 *p_texture2_buffer = NULL;
	Image::RGBA *p_src;
	Image::RGBA *p_dst;

	if (paletted)
	{
		// Try temp 32-bit images first
		p_dst = (Image::RGBA *) mp_cur_temp_32bit_image;
		p_src = (Image::RGBA *) p_ps2_texture->mp_cur_temp_32bit_image;

		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

		// Copy original textures iton temp buffers in 32-bit format
		if (!p_dst)
		{
			p_texture1_buffer = new uint32[m_width * m_height];
			copy_region_to_32bit_buffer(p_texture1_buffer, 0, 0, m_width, m_height);
			p_dst = (Image::RGBA *) p_texture1_buffer;
		}

		if (!p_src)
		{
			p_texture2_buffer = new uint32[p_ps2_texture->m_width * p_ps2_texture->m_height];
			p_ps2_texture->copy_region_to_32bit_buffer(p_texture2_buffer, 0, 0, p_ps2_texture->m_width, p_ps2_texture->m_height);
			p_src = (Image::RGBA *) p_texture2_buffer;
		}

		Mem::Manager::sHandle().PopContext();
	}
	else
	{
		p_src = (Image::RGBA *) p_ps2_texture->mp_single_texture->GetTextureBuffer();
		p_dst = (Image::RGBA *) mp_single_texture->GetTextureBuffer();
	}

	// Do the actual alpha blend
	uint8 om_src_alpha;
	int size = m_width * m_height;

	for (int i = 0; i < size; i++, p_src++, p_dst++)
	{
		om_src_alpha = 0x80 - p_src->a;

		p_dst->r = (uint8) ( ( ((int) p_src->r * p_src->a) + ((int) p_dst->r * om_src_alpha) ) >> 7 /* divide by 0x80 */);
		p_dst->g = (uint8) ( ( ((int) p_src->g * p_src->a) + ((int) p_dst->g * om_src_alpha) ) >> 7 /* divide by 0x80 */);
		p_dst->b = (uint8) ( ( ((int) p_src->b * p_src->a) + ((int) p_dst->b * om_src_alpha) ) >> 7 /* divide by 0x80 */);
		p_dst->a = max(p_src->a, p_dst->a);	// Choose the highest alpha (since we don't want solid pixels to become transparent)
	}

	// Go back to original bitdepth if necessary
	if (paletted && !mp_cur_temp_32bit_image)
	{
		Dbg_Assert(m_clut_bitdepth == 32);

		// Generate a new palette
		if (palette_gen)
		{
			GeneratePalette((Image::RGBA *) mp_single_texture->GetClutBuffer(), (uint8 *) p_texture1_buffer, m_width, m_height, 32, 1 << m_bitdepth);
		}

		// And repalettize and put back into texture
		blit_32bit_buffer_to_texture(p_texture1_buffer, 0, 0, m_width, m_height);

	}

	// Free buffers
	if (p_texture1_buffer)
	{
		delete [] p_texture1_buffer;
	}
	if (p_texture2_buffer)
	{
		delete [] p_texture2_buffer;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CTexDict

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2TexDict::CPs2TexDict(uint32 checksum) : CTexDict(checksum)
{
	// Load nothing
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2TexDict::CPs2TexDict(const char *p_tex_dict_name, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup) : CTexDict(p_tex_dict_name, !is_level_data)
{								  
	LoadTextureDictionary(p_tex_dict_name, NULL, 0, is_level_data, texDictOffset, isSkin, forceTexDictLookup);	// the derived class will does this
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2TexDict::~CPs2TexDict()
{
	UnloadTextureDictionary();				// the derived class does this
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

NxPs2::sScene *	CPs2TexDict::GetEngineTextureDictionary() const
{
	//printf( "Returning scene from GetEngineTextureDictionary %08x\n", (uint32)mp_tex_dict );

	return mp_tex_dict;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2TexDict::add_textures_to_hash_table()
{
	for (int i = 0; i < mp_tex_dict->NumTextures; i++)
	{
		// Make sure we don't add duplicate textures.  We can have the same texture in two
		// different texture groups (for sorting purposes).  This will break texture
		// replacement, though.
		if (!GetTexture(mp_tex_dict->pTextures[i].Checksum))
		{
			CPs2Texture *p_texture = new CPs2Texture;

			p_texture->m_checksum		= mp_tex_dict->pTextures[i].Checksum;
			p_texture->m_width			= mp_tex_dict->pTextures[i].GetWidth();
			p_texture->m_height			= mp_tex_dict->pTextures[i].GetHeight();
			p_texture->m_bitdepth		= mp_tex_dict->pTextures[i].GetBitdepth();
			p_texture->m_clut_bitdepth	= mp_tex_dict->pTextures[i].GetClutBitdepth();
			p_texture->m_num_mipmaps	= mp_tex_dict->pTextures[i].GetNumMipmaps();

			p_texture->mp_group_texture = &(mp_tex_dict->pTextures[i]);

			AddTexture(p_texture);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2TexDict::LoadTextureDictionary(const char *p_tex_dict_name, uint32* pData, int dataSize, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup)
{
	// set up the texture dictionary's main parameters

	bool IsSkin;
	bool IsInstanceable;
	bool UsesPip;

	if ( is_level_data )
	{
		// for level data
		IsSkin = false;
		IsInstanceable = false;
		UsesPip = true;
	}
	else if ( !isSkin && texDictOffset==0 )
	{
		// for non-skinned models (unless they 
		// do texture replacement, such as the
		// boards in the skateshop do)
		IsSkin = false;
		IsInstanceable = false;
		UsesPip = true;
	}   
	else
	{
		// for all skinned models, and for
		// non-skinned models that do
		// texture replacement)
		IsSkin = true;
		IsInstanceable = true;
		UsesPip = false;
	}

	if ( !is_level_data && isSkin )
	{
		// add all skinned model textures
		// to the hash table
		forceTexDictLookup = true;
	}

	if ( p_tex_dict_name )
	{
		// either the filename OR the data pointer should have been specified, but not both
		Dbg_MsgAssert( !pData, ( "You can't specify both a filename %s AND a data pointer", p_tex_dict_name ) )
		
		// p_tex_dict_name is assumed to be the full path name of the texture dictionary
		mp_tex_dict = NxPs2::LoadTextures(p_tex_dict_name, IsSkin, IsInstanceable, UsesPip, texDictOffset, &m_file_size );
		Dbg_Assert( mp_tex_dict );
	}
	else
	{
		Dbg_MsgAssert( pData, ( "No data pointer specified" ) );
		
		m_file_size = dataSize;
		mp_tex_dict = NxPs2::LoadTextures(pData, dataSize, IsSkin, IsInstanceable, UsesPip, texDictOffset );
		Dbg_Assert( mp_tex_dict );
	}

	// Add textures to hash table (just do it for non-level dictionaries now)
	// should this go on bottom up heap?
	if ( forceTexDictLookup )
	{
		add_textures_to_hash_table();
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2TexDict::UnloadTextureDictionary()
{
	NxPs2::DeleteTextures(mp_tex_dict);
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *		CPs2TexDict::plat_load_texture(const char *p_texture_name, bool sprite, bool alloc_vram)
{
	CPs2Texture *p_texture = new CPs2Texture;
	if (!p_texture->LoadTexture(p_texture_name, sprite, alloc_vram))
	{
		Dbg_Error("Can't load texture %s", p_texture_name);
	}

	return p_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *		CPs2TexDict::plat_load_texture_from_buffer(uint8* p_buffer, int buffer_size, uint32 texture_checksum, bool sprite, bool alloc_vram)
{
	CPs2Texture *p_texture = new CPs2Texture;
	if (!p_texture->LoadTextureFromBuffer(p_buffer, buffer_size, texture_checksum, sprite, alloc_vram))
	{
		Dbg_MsgAssert(0, ("Can't load texture from buffer %s", Script::FindChecksumName(texture_checksum)));
	}

	return p_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *		CPs2TexDict::plat_reload_texture(const char *p_texture_name)
{
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2TexDict::plat_unload_texture(CTexture *p_texture)
{
	delete p_texture;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2TexDict::plat_add_texture(CTexture *p_texture)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2TexDict::plat_remove_texture(CTexture *p_texture)
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *		CPs2TexDict::plat_copy_texture(uint32 new_texture_checksum, CTexture *p_texture)
{
	CPs2Texture *p_ps2_texture = static_cast<CPs2Texture *>(p_texture);

	return new CPs2Texture(*p_ps2_texture);
}

} // Namespace Nx  			
				
				
