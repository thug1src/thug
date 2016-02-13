#include <xtl.h>
#include <xgraphics.h>
#include <core\defines.h>
#include <core\crc.h>
#include <sys\file\filesys.h>
#include <gfx\xbox\p_nxtexture.h>
#include "nx_init.h"
#include "render.h"
#include "grass.h"

static bool				bumpsLoaded = false;
NxXbox::sMaterial		waterMaterial;
NxXbox::sTexture		waterTextures[2];

IDirect3DTexture8		*pBumpTextures[17];
NxXbox::sTexture		bumpTextures[17];


namespace NxXbox
{

}



/******************************************************************/
/*                                                                */
/* Unswizzles a 2D texture before it gets unlocked.				  */
/* Note: this operation can be very slow.						  */
/*                                                                */
/******************************************************************/
VOID XBUtil_UnswizzleTexture2D( D3DLOCKED_RECT* pLock, const D3DSURFACE_DESC* pDesc )
{
    DWORD dwPixelSize   = XGBytesPerPixelFromFormat( pDesc->Format );
    DWORD dwTextureSize = pDesc->Width * pDesc->Height * dwPixelSize;

    BYTE* pSrcBits = new BYTE[ dwTextureSize ];
    memcpy( pSrcBits, pLock->pBits, dwTextureSize );
    
    XGUnswizzleRect( pSrcBits, pDesc->Width, pDesc->Height, NULL, pLock->pBits, 
                     0, NULL, dwPixelSize );

    delete [] pSrcBits;
}



/******************************************************************/
/*                                                                */
/* Swizzles a 2D texture before it gets unlocked.				  */
/* Note: this operation can be very slow.						  */
/*                                                                */
/******************************************************************/
VOID XBUtil_SwizzleTexture2D( D3DLOCKED_RECT* pLock, const D3DSURFACE_DESC* pDesc )
{
    DWORD dwPixelSize   = XGBytesPerPixelFromFormat( pDesc->Format );
    DWORD dwTextureSize = pDesc->Width * pDesc->Height * dwPixelSize;

    BYTE* pSrcBits = new BYTE[ dwTextureSize ];
    memcpy( pSrcBits, pLock->pBits, dwTextureSize );
    
    XGSwizzleRect( pSrcBits, 0, NULL, pLock->pBits,
                  pDesc->Width, pDesc->Height, 
                  NULL, dwPixelSize );

    delete [] pSrcBits;
}



/******************************************************************/
/*                                                                */
/* There are 17 frames. Each has 3 level mipmaps				  */
/*                                                                */
/******************************************************************/
HRESULT LoadBumpTextures( void )
{
	HRESULT hr = S_OK;
	for( int i = 0; i < 17; ++i )
    {
		int height = 128, width = 128;

        // Find file name
		char strBumpFile[128];
		sprintf( strBumpFile, "c:\\skate5\\data\\images\\miscellaneous\\%d.bum", i );
        
        // Open and read file.
		void *fp = File::Open( strBumpFile, "rb" );
        if( !fp )
            return E_FAIL;
      
		// Create texture.
		if( FAILED( hr = D3DDevice_CreateTexture( width,
            height,
            3,
            NULL,
            D3DFMT_V8U8,
            D3DPOOL_MANAGED,
			&pBumpTextures[i] )))
            return hr;
       
        // Lock first level.
        D3DLOCKED_RECT lr; 
        D3DSURFACE_DESC desc;
        pBumpTextures[i]->GetLevelDesc( 0, &desc );
        pBumpTextures[i]->LockRect( NULL, &lr, NULL, NULL );
        XBUtil_UnswizzleTexture2D( &lr, &desc );

		File::Read( lr.pBits, sizeof( WORD ), height * width, fp );

		// Modulate the size of the offset.
		uint8 *p_mod = (uint8*)lr.pBits;
		for( int p = 0; p < height * width * 2; ++p )
		{
			// Noticed occasionally this maps to outside of the map tetxure with texbem.
			// Culprit appears to be the maximum negative value (0, given 0x80 offset of values
			// in V8U8 format).
			if( *p_mod == 0 )
			{
				// Max negative offset.
				*p_mod = 1;
			}
			++p_mod;
		}
		
		XBUtil_SwizzleTexture2D( &lr, &desc );
        pBumpTextures[i]->UnlockRect( 0 );

        // Load mipmaps
		int level = 0;
        while( ++level < 3 )
        {
            width >>= 1;
            height >>= 1;
                        
            pBumpTextures[i]->GetLevelDesc( level, &desc );
            pBumpTextures[i]->LockRect( level, &lr, NULL,NULL );
            XBUtil_UnswizzleTexture2D( &lr, &desc );

			File::Read( lr.pBits, sizeof( WORD ), height * width, fp );

			// Modulate the size of the offset.
			p_mod = (uint8*)lr.pBits;
			for( int p = 0; p < height * width * 2; ++p )
			{
				// Noticed occasionally this maps to outside of the map tetxure with texbem.
				// Culprit appears to be the maximum negative value (0, given 0x80 offset of values
				// in V8U8 format).
				if( *p_mod == 0 )
				{
					// Max negative offset.
					*p_mod = 1;
				}
				++p_mod;
			}
			
			XBUtil_SwizzleTexture2D( &lr, &desc );
            pBumpTextures[i]->UnlockRect( level );
        }
		File::Close( fp );

		bumpTextures[i].pD3DTexture = pBumpTextures[i];
		bumpTextures[i].pD3DPalette	= NULL;
	}

	// Set up the texture container for the bump textures.
	ZeroMemory( &waterTextures[0],	sizeof( NxXbox::sTexture ));
	waterTextures[0].pD3DTexture = pBumpTextures[0];

	return hr;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CreateWaterMaterial( NxXbox::sMaterial *p_material )
{
	// Set texture 0 to be the first bump texture.
	p_material->mp_tex[0]			= &waterTextures[0];
	p_material->mp_tex[1]			= &waterTextures[1];
	p_material->m_reg_alpha[0]		= NxXbox::vBLEND_MODE_BLEND_FIXED | ( 0x40UL << 24 );
	p_material->m_color[0][3]		= 0.5f;
	p_material->m_passes			= 2;

	// Add transparent flag.
	p_material->m_flags[0]			|= 0x40;
	p_material->m_flags[1]			= 0x00;

	// Add bump setup flag.
	p_material->m_flags[0]			|= MATFLAG_BUMP_SIGNED_TEXTURE;
	p_material->m_flags[1]			|= MATFLAG_BUMP_LOAD_MATRIX;

	p_material->m_uv_wibble			= false;
	p_material->m_flags[0]			&= ~MATFLAG_ENVIRONMENT;
	p_material->m_flags[0]			&= ~MATFLAG_UV_WIBBLE;

	// Adjust K.
	p_material->m_k[0]				= 0.0f;
	p_material->m_k[1]				= 0.0f;

	// Set texture address mode.
	p_material->m_uv_addressing[0]	= 0x00000000UL;
	p_material->m_uv_addressing[1]	= 0x00000000UL;

	// Set the draw order to ensure meshes are drawn from the bottom up. Default draw order for transparent
	// meshes is 1000.0, so add a little onto that.
	p_material->m_sorted			= false;
	p_material->m_draw_order		= 1501.0f;

	// Create the animating texture details.
	p_material->m_texture_wibble	= true;
	p_material->m_flags[0]			|= MATFLAG_PASS_TEXTURE_ANIMATES;

	p_material->mp_wibble_texture_params						= new NxXbox::sTextureWibbleParams;
	p_material->mp_wibble_texture_params->m_num_keyframes[0]	= 18;
	p_material->mp_wibble_texture_params->m_phase[0]			= 0;
	p_material->mp_wibble_texture_params->m_num_iterations[0]	= 0;
	p_material->mp_wibble_texture_params->mp_keyframes[0]		= new NxXbox::sTextureWibbleKeyframe[18];
	p_material->mp_wibble_texture_params->mp_keyframes[1]		= NULL;
	p_material->mp_wibble_texture_params->mp_keyframes[2]		= NULL;
	p_material->mp_wibble_texture_params->mp_keyframes[3]		= NULL;

	for( int f = 0; f < 18; ++f )
	{
		p_material->mp_wibble_texture_params->mp_keyframes[0][f].m_time						= (( f * 1000 ) / 30 );
		p_material->mp_wibble_texture_params->mp_keyframes[0][f].mp_texture					= &bumpTextures[f % 17];
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool AddWater( Nx::CXboxGeom *p_geom, NxXbox::sMesh *p_mesh )
{
	// Ensure there is at least two sets of uv's available.
	Dbg_Assert( p_mesh->m_uv0_offset > 0 );

	// Check to see whether the texture we want is present.
	Nx::CScene		*p_sky_scene		= Nx::CEngine::sGetSkyScene();
	Nx::CTexDict	*p_tex_dict			= p_sky_scene->GetTexDict();
	Nx::CTexture	*p_texture			= p_tex_dict->GetTexture( 0x96d0bf08UL );

	if( p_texture == NULL )
		return false;

	Nx::CXboxTexture	*p_xbox_texture = static_cast<Nx::CXboxTexture*>( p_texture );
	NxXbox::sTexture	*p_engine_texture	= p_xbox_texture->GetEngineTexture();
	CopyMemory( &waterTextures[1], p_engine_texture, sizeof( NxXbox::sTexture ));

	int num_tc_sets = ( p_mesh->m_vertex_stride - p_mesh->m_uv0_offset ) / 8;
	if( num_tc_sets == 1 )
	{
		// We need to alter this mesh to create two sets of uv's.
		IDirect3DVertexBuffer8* p_new_buffer = p_mesh->AllocateVertexBuffer(( p_mesh->m_vertex_stride + 8 ) * p_mesh->m_num_vertices );
		BYTE* p_read_byte;
		BYTE* p_write_byte;
		p_mesh->mp_vertex_buffer[0]->Lock( 0, 0, &p_read_byte, D3DLOCK_READONLY | D3DLOCK_NOFLUSH );
		p_new_buffer->Lock( 0, 0, &p_write_byte, D3DLOCK_NOFLUSH );

		for( int v = 0; v < p_mesh->m_num_vertices; ++v )
		{
			// Copy all existing vertex information.
			CopyMemory( p_write_byte, p_read_byte, p_mesh->m_vertex_stride );
			p_read_byte		+= p_mesh->m_vertex_stride;
			p_write_byte	+= p_mesh->m_vertex_stride + 8;
		}

		delete p_mesh->mp_vertex_buffer[0];
		p_mesh->mp_vertex_buffer[0] = p_new_buffer;

		p_mesh->m_vertex_stride += 8;

		// Switch the vertex shader.
		p_mesh->m_vertex_shader[0]	&= ~( D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ));
		p_mesh->m_vertex_shader[0]	|= D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2( 0 ) | D3DFVF_TEXCOORDSIZE2( 1 );
	}

	NxXbox::sMaterial *p_water_mat = p_mesh->mp_material;

	CreateWaterMaterial( p_water_mat );

	// Disable skater shadow on the mesh.
	p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
	p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_BUMPED_WATER;

	// Set the water pixel shader.
	p_mesh->m_pixel_shader	= PixelShaderBumpWater;
		
	// Go through and calculate new texture coordinates based on the x-z space of the vertex positions.
	BYTE *p_byte_dest;
	p_mesh->mp_vertex_buffer[0]->Lock( 0, 0, &p_byte_dest, 0 );
	for( uint32 vert = 0; vert < p_mesh->m_num_vertices; ++vert )
	{
		D3DXVECTOR3 *p_pos	= (D3DXVECTOR3*)( p_byte_dest + ( vert * p_mesh->m_vertex_stride ));

		float u0				= p_pos->x / 192.0f;
		float v0				= p_pos->z / 192.0f;

		float u1				= p_pos->x / 96.0f;
		float v1				= p_pos->z / 96.0f;


		float *p_tex		= (float*)( p_byte_dest + p_mesh->m_uv0_offset + ( vert * p_mesh->m_vertex_stride ));
		p_tex[0]			= u0;
		p_tex[1]			= v0;
		p_tex[2]			= u1;
		p_tex[3]			= v1;
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool AddGrass( Nx::CXboxGeom *p_geom, NxXbox::sMesh *p_mesh )
{
	// Need a material to proceed.
	if( p_mesh->mp_material == NULL )
		return false;

	if( p_mesh->mp_material->m_flags[0] & MATFLAG_WATER_EFFECT )
	{
		return AddWater( p_geom, p_mesh );
	}

	if( p_mesh->mp_material->m_grass_layers == 0 )
	{
		return false;
	}

	// At this stage we know we want to add grass.
	Dbg_Assert( p_mesh->mp_material->m_grass_layers > 0 );

	int		grass_layers		= ( p_mesh->mp_material->m_grass_layers > 5 ) ? 5 : p_mesh->mp_material->m_grass_layers;
	float	height_per_layer	= p_mesh->mp_material->m_grass_height / grass_layers;
	
	for( int layer = 0; layer < grass_layers; ++layer )
	{
		Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().BottomUpHeap());
		NxXbox::sMesh *p_grass_mesh = p_mesh->Clone( false );
		Mem::Manager::sHandle().PopContext();
		
		// Disable skater shadow on the mesh.
		p_grass_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
		
		// Turn off anisotropic filtering and z-write on the mesh.
		p_grass_mesh->m_flags |= ( NxXbox::sMesh::MESH_FLAG_NO_ANISOTROPIC | NxXbox::sMesh::MESH_FLAG_NO_ZWRITE );
		
		// Now point the mesh to a different material, first build the material checksum from the name of the parent
		// material (Grass) and the name of the sub material (Grass_LayerX)...
		char material_name[64];
		sprintf( material_name, "Grass-Grass_Layer%d", layer );
		uint32 material_checksum	= Crc::GenerateCRCCaseSensitive( material_name, strlen( material_name ));

		// ...then use the checksum to lookup the material.
		p_grass_mesh->mp_material	= p_geom->mp_scene->GetEngineScene()->GetMaterial( material_checksum );
		Dbg_MsgAssert( p_grass_mesh->mp_material, ( "Grass maaterial for layer %d appears to be named badly", layer ));
		
		// We need also to override the mesh pixel shader, which may have been setup for a multipass material.
		NxXbox::GetPixelShader( p_grass_mesh->mp_material, &p_grass_mesh->m_pixel_shader );
		
		// Add transparent flag to the material.
		p_grass_mesh->mp_material->m_flags[0]		|= 0x40;

		// Adjust K for the material.
		p_grass_mesh->mp_material->m_k[0]			= -2.0f;

		// Set the draw order to ensure meshes are drawn from the bottom up. Default draw order for transparent
		// meshes is 1000.0, so add a little onto that.
		p_grass_mesh->mp_material->m_sorted		 = false;
		p_grass_mesh->mp_material->m_draw_order	 = 1100.0f + ( layer * 0.1f );
		
		// Go through and move all the vertices up a little, and calculate new texture coordinates based on the x-z space of the vertex positions.
		BYTE *p_byte_dest;
		p_grass_mesh->mp_vertex_buffer[0]->Lock( 0, 0, &p_byte_dest, 0 );

		for( uint32 vert = 0; vert < p_grass_mesh->m_num_vertices; ++vert )
		{
			D3DXVECTOR3 *p_pos = (D3DXVECTOR3*)( p_byte_dest + ( vert * p_grass_mesh->m_vertex_stride ));
			p_pos->y += ( height_per_layer * ( layer + 1 ));

			float u			= p_pos->x / 48.0f;
			float v			= p_pos->z / 48.0f;
			float *p_tex	= (float*)( p_byte_dest + p_grass_mesh->m_uv0_offset + ( vert * p_grass_mesh->m_vertex_stride ));
			p_tex[0]		= u;		
			p_tex[1]		= v;
		}

		p_geom->AddMesh( p_grass_mesh );
	}

	// Turn off anisotropic filtering on the base mesh also.
	p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_NO_ANISOTROPIC;
	
	return true;
}
