///////////////////////////////////////////////////////////////////////////////
// NxTexture.cpp

#include <core/defines.h>

#include <core/crc.h>
#include <core/math/vector.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/symboltable.h>
									 
#include <gfx/nx.h>
#include <gfx/nxtexture.h>

namespace	Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static uint32 s_convert_filename_to_checksum( const char* pFileName )
{
	// Find base name
	int idx	= strlen(pFileName);
	while ((idx > 0) && pFileName[idx - 1] != '\\' && pFileName[idx - 1] != '/')
		--idx;

	const char *p_base_name = &(pFileName[idx]);

	return Crc::GenerateCRCFromString(p_base_name);
}

/////////////////////////////////////////////////////////////////////////////
// CTexture

/////////////////////////////////////////////////////////////////////////////
// These functions are the platform independent part of the interface.
					 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture::CTexture()
{
	#ifdef	__NOPT_ASSERT__
	mp_name = NULL;
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture::CTexture(const CTexture & src_texture)
{
	m_checksum = src_texture.m_checksum;
	m_perm = src_texture.m_perm;

	#ifdef	__NOPT_ASSERT__
	mp_name = new char[strlen("Copy of ")+strlen(src_texture.mp_name)+1];
	sprintf (mp_name,"Copy of %s",src_texture.mp_name);	
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture::~CTexture()
{
	#ifdef	__NOPT_ASSERT__
	if (mp_name)
	{
		delete [] mp_name;
	}
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::LoadTexture(const char *p_texture_name, bool sprite, bool alloc_vram)
{								 	
#ifdef	__NOPT_ASSERT__
	mp_name = new char[strlen(p_texture_name)+1];
	sprintf (mp_name,"%s",p_texture_name);	
#endif

	char texture_name[512];
	sprintf(texture_name,"%s",p_texture_name);
	if (sprite)
	{

		if (p_texture_name[0]=='.'
			&& p_texture_name[1]=='.'
			&& p_texture_name[2]=='/')
		{
			// detected a ../ at the start of the file name, so use the full file name
			sprintf(texture_name,"%s",p_texture_name+3);
			
		}
		else
		{
			sprintf(texture_name,"images/%s",p_texture_name);
		}
	}
	
	m_checksum = s_convert_filename_to_checksum( texture_name );
	
	return plat_load_texture(texture_name, sprite, alloc_vram);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::LoadTextureFromBuffer(uint8* p_buffer, int buffer_size, uint32 texture_checksum, bool sprite, bool alloc_vram)
{								 	
#ifdef	__NOPT_ASSERT__
	const char* p_name = Script::FindChecksumName(texture_checksum);
	mp_name = new char[strlen(p_name)+1];
	sprintf (mp_name,"%s",p_name);	
#endif
	m_checksum = texture_checksum;
	return plat_load_texture_from_buffer(p_buffer, buffer_size, sprite, alloc_vram);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::ReplaceTexture(CTexture *p_texture)
{								 	
	return plat_replace_texture(p_texture);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::Generate32BitImage(bool renderable, bool store_original)
{
	return plat_generate_32bit_image(renderable, store_original);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::Put32BitImageIntoTexture(bool new_palette)
{
	return plat_put_32bit_image_into_texture(new_palette);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::Offset(int x_pixels, int y_pixels, bool use_fill_color, Image::RGBA fill_color)
{
	Dbg_MsgAssert(abs(x_pixels) < GetWidth(), ("x_pixels is out of range: %d", x_pixels));
	Dbg_MsgAssert(abs(y_pixels) < GetHeight(), ("y_pixels is out of range: %d", y_pixels));

	return plat_offset(x_pixels, y_pixels, use_fill_color, fill_color);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::AdjustRegion(uint16 x_pos, uint16 y_pos, uint16 width, uint16 height,
							   int split_axis, uint16 start_point, uint16 end_point)
{
	Dbg_MsgAssert((split_axis >= X) && (split_axis <= Y), ("Split axis is out of range: %d", split_axis));
	Dbg_MsgAssert(x_pos < GetWidth(), ("x_pos is out of range: %d", x_pos));
	Dbg_MsgAssert(y_pos < GetHeight(), ("y_pos is out of range: %d", y_pos));
	Dbg_MsgAssert(width <= GetWidth(), ("width is out of range: %d", width));
	Dbg_MsgAssert(height <= GetHeight(), ("height is out of range: %d", height));

	return plat_adjust_region(x_pos, y_pos, width, height, split_axis, start_point, end_point);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::PullToEdge(uint16 point, int axis, int num_pixels)
{
	Dbg_MsgAssert((axis >= X) && (axis <= Y), ("axis is out of range: %d", axis));

	return plat_pull_to_edge(point, axis, num_pixels);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::PushToPoint(uint16 point, int axis, int num_pixels, bool use_fill_color, Image::RGBA fill_color)
{
	Dbg_MsgAssert((axis >= X) && (axis <= Y), ("axis is out of range: %d", axis));

	return plat_push_to_point(point, axis, num_pixels, use_fill_color, fill_color);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::AdjustBrightness(float brightness_scale, bool force_adjust_current)
{
	return plat_adjust_brightness(brightness_scale, force_adjust_current);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::AdjustHSV(float h, float s, float v, bool force_adjust_current)
{
	return plat_adjust_hsv(h, s, v, force_adjust_current);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::AddToVram()
{								 	
	return plat_add_to_vram();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::RemoveFromVram()
{								 	
	return plat_remove_from_vram();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CTexture::GetChecksum() const
{
	return m_checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint16	CTexture::GetWidth() const
{
	return plat_get_width();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint16	CTexture::GetHeight() const
{
	return plat_get_height();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CTexture::GetBitDepth() const
{
	return plat_get_bitdepth();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CTexture::GetPaletteBitDepth() const
{
	return plat_get_palette_bitdepth();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CTexture::GetNumMipmaps() const
{
	return plat_get_num_mipmaps();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::IsTransparent() const
{
	return plat_is_transparent();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::CombineTextures(CTexture *p_texture, bool palette_gen)
{
	return plat_combine_textures(p_texture, palette_gen);
}

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions are provided here:
// so engine implementors can leave certain functionality until later
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_load_texture(const char *p_texture_name, bool sprite, bool alloc_vram)
{
	printf ("STUB: PlatLoadTexture\n");
	Dbg_Assert(0);
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_load_texture_from_buffer(uint8* p_buffer, int buffer_size, bool sprite, bool alloc_vram)
{
	printf ("STUB: PlatLoadTextureFromBuffer\n");
	Dbg_MsgAssert(0, ("This function was only supposed to be called on the PS2"));
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_replace_texture(CTexture *p_texture)
{								 	
	printf ("STUB: PlatReplaceTexture\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_generate_32bit_image(bool renderable, bool store_original)
{
	printf ("STUB: PlatGenerate32BitImage\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_put_32bit_image_into_texture(bool new_palette)
{
	printf ("STUB: PlatPut32BitImageIntoTexture\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_offset(int x_pixels, int y_pixels, bool use_fill_color, Image::RGBA fill_color)
{
	printf ("STUB: PlatOffset\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_adjust_region(uint16 x_pos, uint16 y_pos, uint16 width, uint16 height,
									 int split_axis, uint16 start_point, uint16 end_point)
{								 	
	printf ("STUB: PlatAdjustRegion\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_pull_to_edge(uint16 point, int axis, int num_pixels)
{
	printf ("STUB: PlatPullToEdge\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_push_to_point(uint16 point, int axis, int num_pixels, bool use_fill_color, Image::RGBA fill_color)
{
	printf ("STUB: PlatPushToPoint\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_adjust_brightness(float brightness_scale, bool force_adjust_current)
{
	printf ("STUB: PlatAdjustBrightness\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_adjust_hsv(float h, float s, float v, bool force_adjust_current)
{
	printf ("STUB: PlatAdjustHSV\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_add_to_vram()
{								 	
	printf ("STUB: PlatAddToVram\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_remove_from_vram()
{								 	
	printf ("STUB: PlatRemoveFromVram\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint16	CTexture::plat_get_width() const
{
	printf ("STUB: PlatGetWidth\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint16	CTexture::plat_get_height() const
{
	printf ("STUB: PlatGetHeight\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CTexture::plat_get_bitdepth() const
{
	printf ("STUB: PlatGetBitDepth\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CTexture::plat_get_palette_bitdepth() const
{
	printf ("STUB: PlatGetPaletteBitDepth\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8	CTexture::plat_get_num_mipmaps() const
{
	printf ("STUB: PlatGetNumMipmaps\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_is_transparent() const
{
	printf ("STUB: PlatIsTransparent\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTexture::plat_combine_textures(CTexture *p_texture, bool palette_gen)
{
	printf ("STUB: PlatCombineTextures\n");
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// CMaterial

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMaterial::CMaterial() : mp_texture(NULL)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA			CMaterial::GetRGBA() const
{
	return plat_get_rgba();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CMaterial::SetRGBA(Image::RGBA rgba)
{
	plat_set_rgba(rgba);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CMaterial::GetTexture() const
{
	return mp_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CMaterial::SetTexture(CTexture *tex)
{
	mp_texture = tex;
	plat_set_texture(tex);
}

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions:

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA 		CMaterial::plat_get_rgba() const
{
	printf ("STUB: PlatGetRGBA\n");
	return Image::RGBA();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CMaterial::plat_set_rgba(Image::RGBA rgba)
{
	printf ("STUB: PlatSetRGBA\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CMaterial::plat_set_texture(CTexture *tex)
{
	printf ("STUB: PlatSetTexture\n");
}

/////////////////////////////////////////////////////////////////////////////
// CTexDict

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict::CTexDict(uint32 checksum, bool create_lookup_table)
{
	m_checksum = checksum;
	m_ref_count = 1;	// Belongs to someone
	
	// Mick: for now, since most texture dictionaries don't need to reference
	// the textures by name, we only created the lookup table when it is needed
	// so we save memory	
	// Gary: Well, actually, the create-a-skater texture dictionaries
	// do need lookup tables, just in case there's texture
	// replacement.  So, this constructor should be rewritten so
	// that it takes a parameter telling it whether it should
	// create the hashtable.
	// Garrett: OK
	if (create_lookup_table)
	{
		mp_texture_lookup = new Lst::HashTable<CTexture>(5);
	}
	else
	{
		mp_texture_lookup = NULL;
	}

	// Derived class loads the actual file
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict::CTexDict(const char* p_tex_dict_name, bool create_lookup_table)
{
	m_checksum = Crc::GenerateCRCFromString(p_tex_dict_name);
	m_ref_count = 1;	// Just belong to one scene

	// Mick: for now, since most texture dictionaries don't need to reference
	// the textures by name, we only created the lookup table when it is needed
	// so we save memory	
	// Gary: Well, actually, the create-a-skater texture dictionaries
	// do need lookup tables, just in case there's texture
	// replacement.  So, this constructor should be rewritten so
	// that it takes a parameter telling it whether it should
	// create the hashtable.
	// Garrett: OK
	if (create_lookup_table)
	{
		mp_texture_lookup = new Lst::HashTable<CTexture>(5);
	}
	else
	{
		mp_texture_lookup = NULL;
	}

	// Derived class loads the actual file
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict::~CTexDict()
{
	// Unload everything
	//plat_unload_texture_dictionary();					// the derived class does this

	FlushTextures(true);

	if (mp_texture_lookup)
	{
		delete mp_texture_lookup;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Flush all non permanent textures
void CTexDict::FlushTextures(bool flush_all)
{
	if (mp_texture_lookup)
	{
		// Delete textures
		mp_texture_lookup->IterateStart();
		CTexture *p_texture = mp_texture_lookup->IterateNext();
		while (p_texture)
		{
			CTexture *p_next = mp_texture_lookup->IterateNext();
			if (flush_all || !p_texture->IsPerm())
			{
				#ifdef	__NOPT_ASSERT__
				if (p_texture->GetName())
				{
//					printf ("deleting particle tex %s\n",p_texture->GetName());
				}
				#endif				
				uint32 checksum = p_texture->GetChecksum();			
				delete p_texture;
				mp_texture_lookup->FlushItem(checksum);			
			}
			else
			{
							#ifdef	__NOPT_ASSERT__
				if (p_texture->GetName())
				{
//					printf ("NOT deleting particle tex %s  perm = %d\n",p_texture->GetName(),p_texture->IsPerm());
				}
				#endif				

			}
			p_texture = p_next;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CTexDict::LoadTexture(const char *p_texture_name, bool sprite, bool alloc_vram, bool perm)
{
	Mem::PushMemProfile((char*)p_texture_name);
	
	
	CTexture *p_texture = plat_load_texture(p_texture_name, sprite, alloc_vram);

	if (p_texture)
	{
		p_texture->SetPerm(perm);
		mp_texture_lookup->PutItem(p_texture->GetChecksum(), p_texture);
		
	} else {
		Dbg_Error("Texture %s not found", p_texture_name);
	}

	Mem::PopMemProfile();
	return p_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture * 			CTexDict::LoadTextureFromBuffer(uint8* p_buffer, int buffer_size, uint32 texture_checksum, bool sprite, bool alloc_vram, bool perm)
{
	Mem::PushMemProfile((char*)"texture from buffer");
	CTexture *p_texture = plat_load_texture_from_buffer(p_buffer, buffer_size, texture_checksum, sprite, alloc_vram);

	if (p_texture)
	{
		p_texture->SetPerm(perm);
		mp_texture_lookup->PutItem(p_texture->GetChecksum(), p_texture);
		
	} else {
		Dbg_MsgAssert(0,("Couldn't load texture %s from buffer",Script::FindChecksumName(texture_checksum)));
	}

	Mem::PopMemProfile();
	return p_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CTexDict::UnloadTexture(CTexture *p_texture)
{
	uint32 checksum = p_texture->GetChecksum();

	// Because the texture might be still being used, we have to wait for the frame to 
	// finish rendering
	Nx::CEngine::sFinishRendering();

	if (plat_unload_texture(p_texture))
	{
		mp_texture_lookup->FlushItem(checksum);
		return true;
	} else {
		Dbg_Error("Cannot unload texture %x", checksum);
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CTexDict::AddTexture(CTexture *p_texture)
{
	Dbg_Assert(mp_texture_lookup);
	Dbg_Assert(p_texture);

	mp_texture_lookup->PutItem(p_texture->GetChecksum(), p_texture);

	plat_add_texture(p_texture);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture* CTexDict::get_source_texture( const char* p_src_texture_name )
{
	uint32 src_checksum = Crc::GenerateCRCFromString( p_src_texture_name );
	//src_checksum *= 24;

	// look for the texture with that checksum
	static int modifiers[] = { 2, 3, 4, 2 * 3, 3 * 4, 2 * 4, 2 * 3 * 4, -1 };
	CTexture* pTexture = NULL;
	for (int idx = 0; modifiers[idx] > 0; idx++)
	{
		uint32 calc_checksum = src_checksum * modifiers[idx];
		pTexture = GetTexture( calc_checksum );

		if (pTexture)
		{
			src_checksum *= calc_checksum;
			break;
		}
	}

	// If we haven't found the texture at this stage, it may be because an auto-generated MIP version was used.
	// In which case, the name will have _auto32m0 appended to it. Try searching for this name instead.
	if( pTexture == NULL )
	{
		char auto32_name[512];
		strcpy( auto32_name, p_src_texture_name );

		// Remove the .png, and add the new suffix.
		int length = strlen( auto32_name );
		if( length > 4 )
		{
			auto32_name[strlen( auto32_name ) - 4] = 0;

			// Append the extra bit.
			strcat( auto32_name, "_auto32m0.png" );
		}
		
		src_checksum = Crc::GenerateCRCFromString( auto32_name );
		for( int idx = 0; modifiers[idx] > 0; idx++ )
		{
			uint32 calc_checksum = src_checksum * modifiers[idx];
			pTexture = GetTexture( calc_checksum );
			if( pTexture )
			{
				src_checksum *= calc_checksum;
				break;
			}
		}
	}

	return pTexture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTexDict::ReplaceTexture(const char* p_src_texture_name, const char* p_dst_texture_name)
{
	CTexture* pTexture = get_source_texture( p_src_texture_name );
	
	if ( pTexture )
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

#ifdef __NOPT_ASSERT__
		if ( Script::GetInt( CRCD(0x2a648514,"cas_artist"), false ) )
		{
			Dbg_Message( "Replacing %s with %s here", p_src_texture_name, p_dst_texture_name );
		}
#endif

		CTexture *p_new_texture = LoadTexture(p_dst_texture_name, false, false);
		Dbg_MsgAssert(p_new_texture, ("Can't find replacement texture %s", p_dst_texture_name));

		bool result = pTexture->ReplaceTexture( p_new_texture );
		Dbg_MsgAssert(result, ("Can't replace texture %s with %s", p_src_texture_name, p_dst_texture_name));

		// We're done with the new texture, get rid of it
		UnloadTexture(p_new_texture);

		Mem::Manager::sHandle().PopContext();

		return result;
	}
	else
	{
		// GJ:  The texture checksum lookup table is only
		// available for sprite tex dicts, not for CAS
		// parts, so I've commented out the following for now...
//		if ( Script::GetInt( "cas_artist", false ) )
//		{
//			Dbg_Message( "Couldn't find %s (%08x) in order to replace texture %s", p_src_texture_name, src_checksum, p_dst_texture_name );
//		}
		
		Dbg_Assert( mp_texture_lookup );

/*
		// For debugging...
		
		if (mp_texture_lookup)
		{
			mp_texture_lookup->IterateStart();
			CTexture *p_texture;
			int count = 0;
			while ((p_texture = mp_texture_lookup->IterateNext()))
			{
				Dbg_Message("Found Checksum #%d %08x", count, p_texture->GetChecksum());
				count++;
			}
			Dbg_Assert( count );
		}
*/
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTexDict::ReplaceTexture( const char* p_src_texture_name, CTexture* p_dst_texture )
{
	CTexture* pTexture = get_source_texture( p_src_texture_name );
	
	if ( pTexture )
	{
		bool result = pTexture->ReplaceTexture( p_dst_texture );
		Dbg_MsgAssert(result, ("Can't replace texture %s with CTexture %p", p_src_texture_name, p_dst_texture));

		return result;
	}
	else
	{
		// GJ:  The texture checksum lookup table is only
		// available for sprite tex dicts, not for CAS
		// parts, so I've commented out the following for now...
//		if ( Script::GetInt( "cas_artist", false ) )
//		{
//			Dbg_Message( "Couldn't find %s (%08x) in order to replace texture %s", p_src_texture_name, src_checksum, p_dst_texture_name );
//		}
		
		Dbg_Assert( mp_texture_lookup );

/*
		// For debugging...
		
		if (mp_texture_lookup)
		{
			mp_texture_lookup->IterateStart();
			CTexture *p_texture;
			int count = 0;
			while ((p_texture = mp_texture_lookup->IterateNext()))
			{
				Dbg_Message("Found Checksum #%d %08x", count, p_texture->GetChecksum());
				count++;
			}
			Dbg_Assert( count );
		}
*/
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CTexDict::CopyTexture(uint32 new_texture_checksum, CTexture *p_texture)
{
	CTexture *p_new_texture = plat_copy_texture(new_texture_checksum, p_texture);

	if (p_new_texture)
	{
		p_new_texture->m_checksum = new_texture_checksum;
		mp_texture_lookup->PutItem(new_texture_checksum, p_new_texture);
	} else {
		Dbg_Error("Could not create new CTexture %x", new_texture_checksum);
	}

	return p_new_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CTexDict::CombineTextures(uint32 new_texture_checksum, CTexture *p_texture1, CTexture *p_texture2, bool palette_gen)
{
	CTexture *p_new_texture = CopyTexture(new_texture_checksum, p_texture1);

	if (p_new_texture)
	{
		p_new_texture->CombineTextures(p_texture2, palette_gen);
	} else {
		Dbg_Error("Could not create new CTexture for CombineTexture of %x", new_texture_checksum);
	}

	return p_new_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CTexDict::GetTexture(uint32 checksum) const
{
	if ( !mp_texture_lookup )
	{
		Dbg_Error( "No texture lookup has been created for this texdict" );
		return NULL;
	}

	return mp_texture_lookup->GetItem(checksum);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CTexDict::GetTexture(const char *p_texture_name) const
{
	return GetTexture(s_convert_filename_to_checksum(p_texture_name));
}


///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions:

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CTexDict::plat_load_texture(const char *p_texture_name, bool sprite, bool alloc_vram)
{
	printf ("STUB: PlatLoadTexture\n");
	Dbg_Assert(0);
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CTexDict::plat_load_texture_from_buffer(uint8* p_buffer, int buffer_size, uint32 texture_checksum, bool sprite, bool alloc_vram)
{
	printf ("STUB: PlatLoadTextureFromBuffer\n");
	Dbg_MsgAssert(0, ("This function was only supposed to be called on the PS2"));
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CTexDict::plat_reload_texture(const char *p_texture_name)
{
	printf ("STUB: PlatReloadTexture\n");
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CTexDict::plat_unload_texture(CTexture *p_texture)
{
	printf ("STUB: PlatUnloadTexture\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CTexDict::plat_add_texture(CTexture *p_texture)
{
	printf ("STUB: PlatAddTexture\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CTexDict::plat_remove_texture(CTexture *p_texture)
{
	printf ("STUB: PlatRemoveTexture\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *			CTexDict::plat_copy_texture(uint32 new_texture_checksum, CTexture *p_texture)
{
	printf ("STUB: PlatCopyTexture\n");
	return NULL;
}

}


