#include <core/defines.h>

#include <gel/collision/collision.h>
#include <gel/collision/colltridata.h>
#include <gel/collision/movcollman.h>
#include <gel/collision/batchtricoll.h>
#include <engine/SuperSector.h>

#include <gfx/nx.h>
#include <gfx/nxtexman.h>
#include <gfx/debuggfx.h>
#include <gfx/NxViewMan.h>
#include <gfx/NxMiscFX.h>

#include <gfx/xbox/p_nxgeom.h>
#include <gfx/xbox/p_nxtexture.h>

#include <gfx/xbox/nx/render.h>

namespace Nx
{

#define	DRAW_DEBUG_LINES		0

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sXboxScreenFlashVert
{
	float		x, y, z;
	float		rhw;
	D3DCOLOR	col;
	float		u, v;
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sXboxSplatVert
{
	D3DXVECTOR3		pos;
	D3DCOLOR		col;
	float			u, v;
};


	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sXboxSplatInstanceDetails : public sSplatInstanceDetails
{
	// Platform specific part.
	NxXbox::CInstance	*mp_instance;
	NxXbox::sMaterial	*mp_material;
	sXboxSplatVert		m_verts[SPLAT_POLYS_PER_MESH * 3];
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sXboxShatterInstanceDetails : public sShatterInstanceDetails
{
	// Platform specific part.

						sXboxShatterInstanceDetails( int num_tris, NxXbox::sMesh *p_mesh );
						~sXboxShatterInstanceDetails( void );
	
	NxXbox::sMesh		*mp_mesh;
	uint8				*mp_vertex_buffer;
};


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sXboxShatterInstanceDetails::sXboxShatterInstanceDetails( int num_tris, NxXbox::sMesh *p_mesh ) : sShatterInstanceDetails( num_tris )
{
	mp_mesh				= p_mesh;
	mp_vertex_buffer	= new uint8[num_tris * 3 * p_mesh->m_vertex_stride];
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sXboxShatterInstanceDetails::~sXboxShatterInstanceDetails( void )
{
	delete [] mp_vertex_buffer;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sXboxSplatInstanceDetails * getDetailsForTextureSplat( NxXbox::sTexture *p_texture )
{
	sXboxSplatInstanceDetails *p_xbox_details;

	Dbg_Assert( p_splat_details_table );
	
	// Check to see whether we have a scene already created for this type of texture splat.
	p_splat_details_table->IterateStart();
	sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();
	while( p_details )
	{
		p_xbox_details					= static_cast<sXboxSplatInstanceDetails*>( p_details );
		NxXbox::sMaterial *p_material	= p_xbox_details->mp_material;
		if( p_material->mp_tex[0] == p_texture )
		{
			// This scene contains a material with the required texture, so use this scene.
			return p_xbox_details;
		}
		p_details = p_splat_details_table->IterateNext();
	}
	
	// Create an (opaque) material used to render the mesh.
	NxXbox::sMaterial *p_material	= new NxXbox::sMaterial();
	p_material->m_flags[0]			= (( p_texture ) ? MATFLAG_TEXTURED : 0 );
	p_material->m_checksum			= (uint32)rand() * (uint32)rand();
	p_material->m_passes			= 1;
	p_material->mp_tex[0]			= p_texture;
	p_material->m_no_bfc			= true;
	p_material->m_zbias				= 1;				// To ensure it will sort above most geometry.
	p_material->m_reg_alpha[0]		= 0x00000005UL;		// Blend for now.
	p_material->m_color[0][0]		= 0x80;
	p_material->m_color[0][1]		= 0x80;
	p_material->m_color[0][2]		= 0x80;
	p_material->m_uv_addressing[0]	= 0x00020002UL;		// We want the texture to border - most efficient for alphakill.
	p_material->m_k[0]				= 0.0f;
	p_material->m_alpha_cutoff		= 1;

	p_xbox_details = new sXboxSplatInstanceDetails;
	p_xbox_details->m_highest_active_splat	= 0;
	p_xbox_details->mp_material				= p_material;
	ZeroMemory( p_xbox_details->m_lifetimes, sizeof( int ) * SPLAT_POLYS_PER_MESH );

	for( int v = 0; v < SPLAT_POLYS_PER_MESH * 3; ++v )
	{
		p_xbox_details->m_verts[v].pos = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
		p_xbox_details->m_verts[v].col = D3DCOLOR_RGBA( 0x80, 0x80, 0x80, 0x80 );
	}
	
	p_splat_details_table->PutItem((uint32)p_xbox_details, p_xbox_details );

	return p_xbox_details;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool subdivide_tri_stack( uint8 **p_write, NxXbox::sMesh *p_mesh )
{
	// Three temporary buffers.
	static uint8 v0[128];
	static uint8 v1[128];
	static uint8 v2[128];

	// Three more temporary buffers.
	static uint8 i01[128];
	static uint8 i12[128];
	static uint8 i20[128];

	// If there are elements on the stack, pop off the top three vertices and subdivide if necessary.
	if( triSubdivideStack.IsEmpty())
	{
		return false;
	}
	
	D3DXVECTOR3	*p_v0 = (D3DXVECTOR3*)v0;
	D3DXVECTOR3	*p_v1 = (D3DXVECTOR3*)v1;
	D3DXVECTOR3	*p_v2 = (D3DXVECTOR3*)v2;
	
	// Stack is LIFO, so Pop() off in reverse order.
	triSubdivideStack.Pop( p_v2 );
	triSubdivideStack.Pop( p_v1 );
	triSubdivideStack.Pop( p_v0 );
	
	// Calculate the area of this tri.
	Mth::Vector p(	p_v1->x - p_v0->x, p_v1->y - p_v0->y, p_v1->z - p_v0->z );
	Mth::Vector q(	p_v2->x - p_v0->x, p_v2->y - p_v0->y, p_v2->z - p_v0->z );
	Mth::Vector r(( p[Y] * q[Z] ) - ( q[Y] * p[Z] ), ( p[Z] * q[X] ) - ( q[Z] * p[X] ), ( p[X] * q[Y] ) - ( q[X] * p[Y] ));
	float area_squared = r.LengthSqr();

	if( area_squared > shatterAreaTest )
	{
		// We need to subdivide this tri. Calculate the three intermediate points.
		int block_size = triSubdivideStack.GetBlockSize();

		memcpy( i01, v0, block_size );
		memcpy( i12, v1, block_size );
		memcpy( i20, v2, block_size );

		// Deal with positions (always present).
		((D3DXVECTOR3*)i01 )->x = p_v0->x + (( p_v1->x - p_v0->x ) * 0.5f );
		((D3DXVECTOR3*)i01 )->y = p_v0->y + (( p_v1->y - p_v0->y ) * 0.5f );
		((D3DXVECTOR3*)i01 )->z = p_v0->z + (( p_v1->z - p_v0->z ) * 0.5f );

		((D3DXVECTOR3*)i12 )->x = p_v1->x + (( p_v2->x - p_v1->x ) * 0.5f );
		((D3DXVECTOR3*)i12 )->y = p_v1->y + (( p_v2->y - p_v1->y ) * 0.5f );
		((D3DXVECTOR3*)i12 )->z = p_v1->z + (( p_v2->z - p_v1->z ) * 0.5f );

		((D3DXVECTOR3*)i20 )->x = p_v2->x + (( p_v0->x - p_v2->x ) * 0.5f );
		((D3DXVECTOR3*)i20 )->y = p_v2->y + (( p_v0->y - p_v2->y ) * 0.5f );
		((D3DXVECTOR3*)i20 )->z = p_v2->z + (( p_v0->z - p_v2->z ) * 0.5f );

		// Deal with colors (not always present).
		if( p_mesh->m_diffuse_offset > 0 )
		{
			uint8	*p_v0col	= (uint8*)( v0 + p_mesh->m_diffuse_offset );
			uint8	*p_v1col	= (uint8*)( v1 + p_mesh->m_diffuse_offset );
			uint8	*p_v2col	= (uint8*)( v2 + p_mesh->m_diffuse_offset );
			uint8	*p_i01col	= (uint8*)( i01 + p_mesh->m_diffuse_offset );
			uint8	*p_i12col	= (uint8*)( i12 + p_mesh->m_diffuse_offset );
			uint8	*p_i20col	= (uint8*)( i20 + p_mesh->m_diffuse_offset );
		
			for( int i = 0; i < 4; ++i )
			{
				p_i01col[i]		= p_v0col[i] + (((int)p_v1col[i] - (int)p_v0col[i] ) / 2 );
				p_i12col[i]		= p_v1col[i] + (((int)p_v2col[i] - (int)p_v1col[i] ) / 2 );
				p_i20col[i]		= p_v2col[i] + (((int)p_v0col[i] - (int)p_v2col[i] ) / 2 );
			}
		}

		// Deal with uv0 (not always present).
		if( p_mesh->m_uv0_offset > 0 )
		{
			float	*p_v0uv		= (float*)( v0 + p_mesh->m_uv0_offset );
			float	*p_v1uv		= (float*)( v1 + p_mesh->m_uv0_offset );
			float	*p_v2uv		= (float*)( v2 + p_mesh->m_uv0_offset );
			float	*p_i01uv	= (float*)( i01 + p_mesh->m_uv0_offset );
			float	*p_i12uv	= (float*)( i12 + p_mesh->m_uv0_offset );
			float	*p_i20uv	= (float*)( i20 + p_mesh->m_uv0_offset );
		
			for( int i = 0; i < 2; ++i )
			{
				p_i01uv[i]		= p_v0uv[i] + (( p_v1uv[i] - p_v0uv[i] ) * 0.5f );
				p_i12uv[i]		= p_v1uv[i] + (( p_v2uv[i] - p_v1uv[i] ) * 0.5f );
				p_i20uv[i]		= p_v2uv[i] + (( p_v0uv[i] - p_v2uv[i] ) * 0.5f );
			}
		}
		
		// Push the four new tris onto the stack.
		triSubdivideStack.Push( v0 );
		triSubdivideStack.Push( i01 );
		triSubdivideStack.Push( i20 );

		triSubdivideStack.Push( i01 );
		triSubdivideStack.Push( v1 );
		triSubdivideStack.Push( i12 );

		triSubdivideStack.Push( i01 );
		triSubdivideStack.Push( i12 );
		triSubdivideStack.Push( i20 );

		triSubdivideStack.Push( i20 );
		triSubdivideStack.Push( i12 );
		triSubdivideStack.Push( v2 );
	}
	else
	{
		// Don't need to subdivide this tri.
		int block_size = triSubdivideStack.GetBlockSize();

		// Just copy the tri into the next available slot.
		memcpy( *p_write, v0, block_size );
		*p_write += block_size;
		memcpy( *p_write, v1, block_size );
		*p_write += block_size;
		memcpy( *p_write, v2, block_size );
		*p_write += block_size;
	}
	return true;
}



/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_screen_flash_render( sScreenFlashDetails *p_details )
{
	// Get viewport details.
	CViewport *p_vp = CViewportManager::sGetActiveViewport( p_details->m_viewport );

	sXboxScreenFlashVert verts[4];

	verts[0].x	= p_vp->GetOriginX() * NxXbox::EngineGlobals.backbuffer_width;
	verts[0].y	= p_vp->GetOriginY() * NxXbox::EngineGlobals.backbuffer_height;
	verts[0].z	= p_details->m_z;
	
	verts[1].x	= verts[0].x + ( p_vp->GetWidth() * NxXbox::EngineGlobals.backbuffer_width );
	verts[1].y	= verts[0].y;
	verts[1].z	= verts[0].z;

	verts[2].x	= verts[0].x + ( p_vp->GetWidth() * NxXbox::EngineGlobals.backbuffer_width );
	verts[2].y	= verts[0].y + ( p_vp->GetHeight() * NxXbox::EngineGlobals.backbuffer_height );
	verts[2].z	= verts[0].z;

	verts[3].x	= verts[0].x;
	verts[3].y	= verts[0].y + ( p_vp->GetHeight() * NxXbox::EngineGlobals.backbuffer_height );
	verts[3].z	= verts[0].z;

	for( int v = 0; v < 4; ++v )
	{
		verts[v].col = D3DCOLOR_ARGB( p_details->m_current.a, p_details->m_current.r, p_details->m_current.g, p_details->m_current.b );
		verts[v].rhw = 1.0f;
	}

	if( p_details->mp_texture )
	{
		verts[0].u	= 0.0f;
		verts[0].v	= 0.0f;
		verts[1].u	= 1.0f;
		verts[1].v	= 0.0f;
		verts[2].u	= 1.0f;
		verts[2].v	= 1.0f;
		verts[3].u	= 0.0f;
		verts[3].v	= 1.0f;

		Nx::CXboxTexture *p_xbox_texture = static_cast<CXboxTexture*>( p_details->mp_texture );

		NxXbox::set_texture( 0, p_xbox_texture->GetEngineTexture()->pD3DTexture, p_xbox_texture->GetEngineTexture()->pD3DPalette );
		NxXbox::set_render_state( RS_UVADDRESSMODE0, 0x00010001UL );
	}
	else	
	{
		NxXbox::set_texture( 0, NULL );
	}
	
	NxXbox::set_blend_mode( NxXbox::vBLEND_MODE_BLEND );

	NxXbox::set_render_state( RS_ZWRITEENABLE,	0 );
	NxXbox::set_render_state( RS_ZTESTENABLE,	0 );

	NxXbox::set_pixel_shader( 0 );
	NxXbox::set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ));
	
	NxXbox::EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_QUADLIST, 1, verts, sizeof( sXboxScreenFlashVert ));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_texture_splat_initialize( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_texture_splat_cleanup( void )
{
	sXboxSplatInstanceDetails *p_xbox_details;

	Dbg_Assert( p_splat_details_table );
	
	p_splat_details_table->IterateStart();
	sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();
	while( p_details )
	{
		p_xbox_details = static_cast<sXboxSplatInstanceDetails*>( p_details );

		delete p_xbox_details->mp_material;
		
		p_details = p_splat_details_table->IterateNext();

		p_splat_details_table->FlushItem((uint32)p_xbox_details );
		delete p_xbox_details;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_texture_splat_reset_poly( sSplatInstanceDetails *p_details, int index )
{
	// Cast the details to Xbox details.
	sXboxSplatInstanceDetails *p_xbox_details = static_cast<sXboxSplatInstanceDetails *>( p_details );
	
	// Force this poly to be degenerate.
	p_xbox_details->m_verts[index * 3 + 1]	= p_xbox_details->m_verts[index * 3];
	p_xbox_details->m_verts[index * 3 + 2]	= p_xbox_details->m_verts[index * 3];
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline float CrossProduct2D( const Mth::Vector& v1, const Mth::Vector& v2 )
{	
	// Assumes for both v1 and v2 that the [z] and [w] components are 0.
	return ( v1[X] * v2[Y] ) - ( v1[Y] * v2[X] );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static bool same_side( Mth::Vector &p1, Mth::Vector &p2, Mth::Vector &a, Mth::Vector &b )
{
	float cp1 = CrossProduct2D( b - a, p1 - a );
	float cp2 = CrossProduct2D( b - a, p2 - a );
	if(( cp1 * cp2 ) >= 0.0f )
		return true;
    else
		return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static bool point_in_triangle( Mth::Vector &p, Mth::Vector &a, Mth::Vector &b, Mth::Vector &c )
{
	if( same_side( p, a, b, c ) && same_side( p, b, c, a ) && same_side( p, c, a, b ))
		return true;
    else
		return false;
}






/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static inline bool line_segment_intersection( float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4 )
{
	float ax = x2 - x1;
	float bx = x3 - x4;
	float ay = y2 - y1;
	float by = y3 - y4;
	float cx = x1 - x3;
	float cy = y1 - y3;
	float d = by * cx - bx * cy;	// Alpha numerator.
	float f = ay * bx - ax * by;	// Both denominator.

	// Alpha tests.
	if( f > 0.0f )
	{
		if( d < 0.0f || d > f )
			return false;
	}
	else
	{
		if( d > 0 || d < f )
			return false;
	}

	float e = ax * cy - ay * cx;	// Beta numerator.
	
	// Beta tests.
	if( f > 0.0f )
	{
		if( e < 0.0f || e > f )
			return false;
	}
	else
	{
		if( e > 0 || e < f )
			return false;
	}

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static inline bool tri_texture_intersect( float u0, float v0, float u1, float v1, float u2, float v2 )
{
	// Trivial check to see if all three points are outside range of texture.
	if(( u0 < -1.0f ) && ( u1 < -1.0f ) && ( u2 < -1.0f ))
		return false;
	if(( u0 > 1.0f ) && ( u1 > 1.0f ) && ( u2 > 1.0f ))
		return false;
	if(( v0 < -1.0f ) && ( v1 < -1.0f ) && ( v2 < -1.0f ))
		return false;
	if(( v0 > 1.0f ) && ( v1 > 1.0f ) && ( v2 > 1.0f ))
		return false;
	
	// Perform the check to see if any line segment forming the tri intersects any line segment forming the texture.
	Mth::Vector texture_square[4] = {	Mth::Vector( -1.0f, -1.0f, 0.0f, 0.0f ),
										Mth::Vector(  1.0f, -1.0f, 0.0f, 0.0f ),
										Mth::Vector(  1.0f,  1.0f, 0.0f, 0.0f ),
										Mth::Vector( -1.0f,  1.0f, 0.0f, 0.0f )};
	for( int p = 0; p < 4; ++p )
	{
		int q = ( p + 1 ) % 4;
		if( line_segment_intersection( u0, v0, u1, v1, texture_square[p][X], texture_square[p][Y], texture_square[q][X], texture_square[q][Y] ))
			return true;
		if( line_segment_intersection( u1, v1, u2, v2, texture_square[p][X], texture_square[p][Y], texture_square[q][X], texture_square[q][Y] ))
			return true;
		if( line_segment_intersection( u2, v2, u0, v0, texture_square[p][X], texture_square[p][Y], texture_square[q][X], texture_square[q][Y] ))
			return true;
	}

	// If we reach this point there are three remaining possibilities:
	// 1) That the tri lies entirely within the texture
	// 2) That the texture lies entirely within the tri
	// 3) That there is no space shared by tri and texture

	// 1) Perform a trivial check to see whether a corner of the tri lies within the texture.
	if(( u0 >= -1.0f ) && ( u0 <= 1.0f ) && ( v0 >= -1.0f ) && ( v0 <= 1.0f ))
		return true;
	if(( u1 >= -1.0f ) && ( u1 <= 1.0f ) && ( v1 >= -1.0f ) && ( v1 <= 1.0f ))
		return true;
	if(( u2 >= -1.0f ) && ( u2 <= 1.0f ) && ( v2 >= -1.0f ) && ( v2 <= 1.0f ))
		return true;

	// 2) Check that at least one corner of the texture falls within the tri.
	Mth::Vector a( u0, v0, 0.0f );
	Mth::Vector b( u1, v1, 0.0f );
	Mth::Vector c( u2, v2, 0.0f );
	for( int p = 0; p < 4; ++p )
	{
		if( point_in_triangle( texture_square[p], a, b, c ))
		{
			return true;
		}
	}

	// 3) No space shared.
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool plat_texture_splat( Nx::CSector **pp_sectors, Nx::CCollStatic **pp_collision, Mth::Vector& start, Mth::Vector& end, float size, float lifetime, Nx::CTexture *p_texture, Nx::sSplatTrailInstanceDetails *p_trail_details )
{
	XGMATRIX view_matrix, ortho_matrix, projection_matrix;

#	if DRAW_DEBUG_LINES
	Gfx::AddDebugLine( start, end, MAKE_RGB( 200, 200, 0 ), MAKE_RGB( 200, 200, 0 ), 1 );
#	endif // DRAW_DEBUG_LINES
	
	// The length of the start->end line defines the view depth of the frustum.
	Mth::Vector	splat_vector	= end - start;
	float		view_depth		= splat_vector.Length();
	splat_vector.Normalize();
	
	// Calculate the parallel projection matrix. Generally the projection vector will tend to point downwards, so we use a
	// random vector in the x-z plane to define the up vector for the projection. However if this splat is part of a trail,
	// use the previous point to generate the up vector.
	Mth::Vector up;
	if( p_trail_details )
	{
		up.Set( start[X] - p_trail_details->m_last_pos[X], start[Y] - p_trail_details->m_last_pos[Y], start[Z] - p_trail_details->m_last_pos[Z] );

		// The height of the viewport is defined by the distance between the two points.
		float height = up.Length() * 0.5f;

		// Now we move start and end halfway back along the up vector.
		start	-= up * 0.5f;
		end		-= up * 0.5f;
		
		up.Normalize();
		
		XGMatrixLookAtRH( &view_matrix, (XGVECTOR3*)( &start[X] ), (XGVECTOR3*)( &end[X] ), (XGVECTOR3*)( &up[X] ));
		XGMatrixOrthoRH( &ortho_matrix, size, height, 0.1f, view_depth );
	}
	else if( fabsf( splat_vector[Y] ) > 0.5f )
	{
		float angle = ((float)rand() * 2.0f * Mth::PI ) / (float)RAND_MAX;
		up.Set( sinf( angle ), 0.0f, cosf( angle ));
		XGMatrixLookAtRH( &view_matrix, (XGVECTOR3*)( &start[X] ), (XGVECTOR3*)( &end[X] ), (XGVECTOR3*)( &up[X] ));
		XGMatrixOrthoRH( &ortho_matrix, size, size, 0.1f, view_depth );
	}
	else
	{
		up.Set( 0.0f, 1.0f, 0.0f );
		XGMatrixLookAtRH( &view_matrix, (XGVECTOR3*)( &start[X] ), (XGVECTOR3*)( &end[X] ), (XGVECTOR3*)( &up[X] ));
		XGMatrixOrthoRH( &ortho_matrix, size, size, 0.1f, view_depth );
	}

	XGMatrixMultiply( &projection_matrix, &view_matrix, &ortho_matrix );
	
	// Pointer to the mesh we will be modifying. (Don't want to set the pointer up until we know for
	// sure that we will be adding some polys).
	sXboxSplatInstanceDetails	*p_details		= NULL;
	sXboxSplatVert				*p_target_verts	= NULL;

	Nx::CSector *p_sector;

	while( p_sector = *pp_sectors )
	{
		Nx::CXboxGeom *p_xbox_geom = static_cast<Nx::CXboxGeom*>( p_sector->GetGeom());

		if( p_xbox_geom )
		{
#			if DRAW_DEBUG_LINES
			Mth::Vector min = p_xbox_geom->GetBoundingBox().GetMin();
			Mth::Vector max = p_xbox_geom->GetBoundingBox().GetMax();

			Mth::Vector box[8];
			box[0] = box[1] = box[2] = box[3] = max;
			box[1][X] = min[X];
			box[2][Y] = min[Y];
			box[3][Z] = min[Z];
			box[5] = box[6] = box[7] = box[4] = min;;
			box[5][X] = max[X];
			box[6][Y] = max[Y];
			box[7][Z] = max[Z];

			for ( int i = 1; i < 4; i++ )
			{
				Gfx::AddDebugLine( box[0], box[i], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
			}
			for ( int i = 5; i < 8; i++ )
			{
				Gfx::AddDebugLine( box[4], box[i], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
			}
			Gfx::AddDebugLine( box[1], box[6], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
			Gfx::AddDebugLine( box[1], box[7], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
			Gfx::AddDebugLine( box[2], box[5], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
			Gfx::AddDebugLine( box[2], box[7], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
			Gfx::AddDebugLine( box[3], box[5], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
			Gfx::AddDebugLine( box[3], box[6], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
#			endif // DRAW_DEBUG_LINES
			
			// For each mesh in the geom...
			for( uint32 m = 0; m < p_xbox_geom->m_num_mesh; ++m )
			{
				NxXbox::sMesh *p_mesh = p_xbox_geom->m_mesh_array[m];

				// Not allowed on meshes which are flagged not to shadow.
				if( p_mesh->m_flags & NxXbox::sMesh::MESH_FLAG_NO_SKATER_SHADOW )
					continue;

				// Check the bounding box of this mesh falls within the scope of the line.
				
				// Transform the mesh bounding box to see whether it falls within the projection frustum.
				
				// If it falls within the projection frustum, we need to explicitly transform all the vertices.
				BYTE *p_vert_data;
				p_mesh->mp_vertex_buffer[0]->Lock( 0, 0, &p_vert_data, D3DLOCK_READONLY );
				
				// Now scan through each non-degenerate tri, checking the verts to see if they are within scope.
				uint32 index0;
				uint32 index1 = p_mesh->mp_index_buffer[0][0];
				uint32 index2 = p_mesh->mp_index_buffer[0][1];
				for( uint32 i = 2; i < p_mesh->m_num_indices[0]; ++i )
				{
					// Wrap the indices round.
					index0 = index1;
					index1 = index2;
					index2 = p_mesh->mp_index_buffer[0][i];

					if(( index0 != index1 ) && ( index0 != index2 ) && ( index1 != index2 ))
					{
						XGVECTOR3 uvprojections[3];
						XGVec3TransformCoord( &uvprojections[0], (XGVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index0 )), &projection_matrix );
						XGVec3TransformCoord( &uvprojections[1], (XGVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index1 )), &projection_matrix );
						XGVec3TransformCoord( &uvprojections[2], (XGVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index2 )), &projection_matrix );

						// Check the z-values here, everything else is checked in tri_texture_intersect().
						if(( uvprojections[0].z < 0.0f ) && ( uvprojections[1].z < 0.0f ) && ( uvprojections[2].z < 0.0f ))
							continue;
						if(( uvprojections[0].z > 1.0f ) && ( uvprojections[1].z > 1.0f ) && ( uvprojections[2].z > 1.0f ))
							continue;
						
						// Okay, this tri lies within the projection frustum. Now check that it intersects the texture
						if( !tri_texture_intersect( uvprojections[0].x, uvprojections[0].y,
													uvprojections[1].x, uvprojections[1].y,
													uvprojections[2].x, uvprojections[2].y ))
						{
							continue;
						}
						
						// Get a pointer to the mesh used for rendering texture splats with the given texture.
						// (Note this will create a new instance to handle texture splats of this texture if one does not already exist).
						if( p_target_verts == NULL )
						{
							CXboxTexture *p_xbox_texture	= static_cast<CXboxTexture*>( p_texture );
							p_details						= getDetailsForTextureSplat( p_xbox_texture->GetEngineTexture());
							p_target_verts					= p_details->m_verts;
							Dbg_Assert( p_target_verts );
						}
						
						// Scan through the lifetimes, finding a 'dead' poly (lifetime == 0), or the oldest.
						uint32 idx						= p_details->GetOldestSplat();

						// Convert lifetime from seconds to milliseconds.
						p_details->m_lifetimes[idx]		= (int)( lifetime * 1000.0f );

						// Set up the corresponding vertices. First write the positions...
						uint32 index					= idx * 3;
						p_target_verts[index + 0].pos	= *(D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index0 ));
						p_target_verts[index + 1].pos	= *(D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index1 ));
						p_target_verts[index + 2].pos	= *(D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index2 ));

						// ...then the uv's...
						p_target_verts[index + 0].u		= ( uvprojections[0].x + 1.0f ) * 0.5f;
						p_target_verts[index + 0].v		= ( uvprojections[0].y + 1.0f ) * 0.5f;
						p_target_verts[index + 1].u		= ( uvprojections[1].x + 1.0f ) * 0.5f;
						p_target_verts[index + 1].v		= ( uvprojections[1].y + 1.0f ) * 0.5f;
						p_target_verts[index + 2].u		= ( uvprojections[2].x + 1.0f ) * 0.5f;
						p_target_verts[index + 2].v		= ( uvprojections[2].y + 1.0f ) * 0.5f;

						// ...then the vertex colors.
						p_target_verts[index + 0].col	= ( *(D3DCOLOR*)( p_vert_data + ( p_mesh->m_vertex_stride * index0 ) + p_mesh->m_diffuse_offset ) & 0xFFFFFFUL ) | 0x80000000UL;
						p_target_verts[index + 1].col	= ( *(D3DCOLOR*)( p_vert_data + ( p_mesh->m_vertex_stride * index1 ) + p_mesh->m_diffuse_offset ) & 0xFFFFFFUL ) | 0x80000000UL;
						p_target_verts[index + 2].col	= ( *(D3DCOLOR*)( p_vert_data + ( p_mesh->m_vertex_stride * index2 ) + p_mesh->m_diffuse_offset ) & 0xFFFFFFUL ) | 0x80000000UL;
						
						D3DXVECTOR3	*p_v0 = &( p_target_verts[index + 0].pos );
						D3DXVECTOR3	*p_v1 = &( p_target_verts[index + 1].pos );
						D3DXVECTOR3	*p_v2 = &( p_target_verts[index + 2].pos );
						Mth::Vector pv(	p_v1->x - p_v0->x, p_v1->y - p_v0->y, p_v1->z - p_v0->z );
						Mth::Vector qv(	p_v2->x - p_v0->x, p_v2->y - p_v0->y, p_v2->z - p_v0->z );
						Mth::Vector r(( pv[Y] * qv[Z] ) - ( qv[Y] * pv[Z] ), ( pv[Z] * qv[X] ) - ( qv[Z] * pv[X] ), ( pv[X] * qv[Y] ) - ( qv[X] * pv[Y] ));
						float area_squared = r.LengthSqr();

						// Set the shatter test to ensure that we don't subdivide too far. Note that each successive subdivision will quarter
						// the area of each triangle, which means the area *squared* of each triangle will become 1/16th of the previous value.
						shatterAreaTest = area_squared / 128.0f;
						
						triSubdivideStack.Reset();
						triSubdivideStack.SetBlockSize( sizeof( sXboxSplatVert ));
						triSubdivideStack.Push( &p_target_verts[index + 0] );
						triSubdivideStack.Push( &p_target_verts[index + 1] );
						triSubdivideStack.Push( &p_target_verts[index + 2] );

						// Allocate a block of memory into which the subdivision stack will write the results.
						uint8			*p_array		= new uint8[8 * 1024];
						uint8			*p_array_start	= p_array;
						uint8			*p_array_loop	= p_array;
						memset( p_array, 0, 8 * 1024 );

						NxXbox::sMesh	*p_dummy_mesh	= new NxXbox::sMesh();
						p_dummy_mesh->m_diffuse_offset	= 12;
						p_dummy_mesh->m_uv0_offset		= 16;

						while( subdivide_tri_stack( &p_array, p_dummy_mesh ));

						// Ensure we haven't overrun the buffer.
						Dbg_Assert((uint32)p_array - (uint32)p_array_loop < ( 8 * 1024 ));

						bool subdivided_tri_added = false;

						while( p_array_loop != p_array )
						{
							// Add this triangle, *if* it is valid.
							if( tri_texture_intersect((((sXboxSplatVert*)p_array_loop )[0].u * 2.0f ) - 1.0f,
													  (((sXboxSplatVert*)p_array_loop )[0].v * 2.0f ) - 1.0f,
													  (((sXboxSplatVert*)p_array_loop )[1].u * 2.0f ) - 1.0f,
													  (((sXboxSplatVert*)p_array_loop )[1].v * 2.0f ) - 1.0f,
													  (((sXboxSplatVert*)p_array_loop )[2].u * 2.0f ) - 1.0f,
													  (((sXboxSplatVert*)p_array_loop )[2].v * 2.0f ) - 1.0f ))
							{
								// We have added at least one subdivided tri.
								subdivided_tri_added = true;

								// Convert lifetime from seconds to milliseconds.
								p_details->m_lifetimes[idx]		= (int)( lifetime * 1000.0f );
							
								p_target_verts[index + 0].pos	= ((sXboxSplatVert*)p_array_loop )[0].pos;
								p_target_verts[index + 1].pos	= ((sXboxSplatVert*)p_array_loop )[1].pos;
								p_target_verts[index + 2].pos	= ((sXboxSplatVert*)p_array_loop )[2].pos;
							
								p_target_verts[index + 0].u		= ((sXboxSplatVert*)p_array_loop )[0].u;
								p_target_verts[index + 0].v		= ((sXboxSplatVert*)p_array_loop )[0].v;
								p_target_verts[index + 1].u		= ((sXboxSplatVert*)p_array_loop )[1].u;
								p_target_verts[index + 1].v		= ((sXboxSplatVert*)p_array_loop )[1].v;
								p_target_verts[index + 2].u		= ((sXboxSplatVert*)p_array_loop )[2].u;
								p_target_verts[index + 2].v		= ((sXboxSplatVert*)p_array_loop )[2].v;

								p_target_verts[index + 0].col	= (((sXboxSplatVert*)p_array_loop )[0].col & 0xFFFFFFUL ) | 0x80000000UL;
								p_target_verts[index + 1].col	= (((sXboxSplatVert*)p_array_loop )[1].col & 0xFFFFFFUL ) | 0x80000000UL;
								p_target_verts[index + 2].col	= (((sXboxSplatVert*)p_array_loop )[2].col & 0xFFFFFFUL ) | 0x80000000UL;
							
								idx								= p_details->GetOldestSplat();
								index							= idx * 3;
							}
							
							p_array_loop						+= ( sizeof( sXboxSplatVert ) * 3 );
						}

						if( !subdivided_tri_added )
						{
							// No subdivided tris were added. This means we still have the large tri sitting in the list, which we don't want.
							p_details->m_lifetimes[idx]			= 0;
							plat_texture_splat_reset_poly( p_details, idx );
						}
						
						delete p_dummy_mesh;
						delete [] p_array_start;

#						if DRAW_DEBUG_LINES
						D3DXVECTOR3* p_d3dvert;
						p_d3dvert = (D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index0 ));
						Mth::Vector v0( p_d3dvert->x, p_d3dvert->y, p_d3dvert->z );
						p_d3dvert = (D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index1 ));
						Mth::Vector v1( p_d3dvert->x, p_d3dvert->y, p_d3dvert->z );
						p_d3dvert = (D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index2 ));
						Mth::Vector v2( p_d3dvert->x, p_d3dvert->y, p_d3dvert->z );
						Gfx::AddDebugLine( v0, v1, MAKE_RGB( 0, 200, 200 ), MAKE_RGB( 0, 200, 200 ), 1 );
						Gfx::AddDebugLine( v1, v2, MAKE_RGB( 0, 200, 200 ), MAKE_RGB( 0, 200, 200 ), 1 );
						Gfx::AddDebugLine( v2, v0, MAKE_RGB( 0, 200, 200 ), MAKE_RGB( 0, 200, 200 ), 1 );
#						endif // DRAW_DEBUG_LINES
					}
				}
			}
		}
		++pp_sectors;
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_texture_splat_render( void )
{
	sXboxSplatInstanceDetails *p_xbox_details;

	Dbg_Assert( p_splat_details_table );

	NxXbox::set_pixel_shader( 0 );	
	NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ));
	
	D3DDevice_SetTextureStageState( 0, D3DTSS_BORDERCOLOR, 0x00000000UL );

	// Store the stage zero minfilter, since it may be anisotropic.
	DWORD stage_zero_minfilter;
	D3DDevice_GetTextureStageState( 0, D3DTSS_MINFILTER, &stage_zero_minfilter );
	D3DDevice_SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );

	p_splat_details_table->IterateStart();
	sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();
	
	while( p_details )
	{
		p_xbox_details = static_cast<sXboxSplatInstanceDetails*>( p_details );

		if( p_xbox_details->m_highest_active_splat >= 0 )
		{
			p_xbox_details->mp_material->Submit();
			NxXbox::EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_TRIANGLELIST, p_xbox_details->m_highest_active_splat + 1, p_xbox_details->m_verts, sizeof( sXboxSplatVert ));
		}
		
		p_details = p_splat_details_table->IterateNext();
	}

	// Restore the stage zero minfilter.
	D3DDevice_SetTextureStageState( 0, D3DTSS_MINFILTER, stage_zero_minfilter );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_shatter_initialize( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_shatter_cleanup( void )
{
	sXboxShatterInstanceDetails *p_xbox_details;

	Dbg_Assert( p_shatter_details_table );
	
	p_shatter_details_table->IterateStart();
	sShatterInstanceDetails *p_details = p_shatter_details_table->IterateNext();
	while( p_details )
	{
		p_xbox_details = static_cast<sXboxShatterInstanceDetails*>( p_details );
		
		p_details = p_shatter_details_table->IterateNext();

		p_shatter_details_table->FlushItem((uint32)p_xbox_details );
		delete p_xbox_details;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_shatter( CGeom *p_geom )
{
	CXboxGeom *p_xbox_geom = static_cast<CXboxGeom*>( p_geom );

	// For each mesh in the geom...
	for( uint32 m = 0; m < p_xbox_geom->m_num_mesh; ++m )
	{
		NxXbox::sMesh *p_mesh = p_xbox_geom->m_mesh_array[m];

		if( p_mesh->m_num_indices[0] >= 3 )
		{
			// Set the block size for this mesh.
			triSubdivideStack.SetBlockSize( p_mesh->m_vertex_stride );
			
			// Get a pointer to the renderable data.
			BYTE *p_vert_data;
			p_mesh->mp_vertex_buffer[0]->Lock( 0, 0, &p_vert_data, D3DLOCK_READONLY );
				
			// First scan through each non-degenerate tri, counting them to see how many verts we'll need.
			// We also have to figure the area of the tris here, since we need to calculate the worst case given the requirements for subdivision.
			uint32 valid_tris	= 0;
			uint32 index0;
			uint32 index1		= p_mesh->mp_index_buffer[0][0];
			uint32 index2		= p_mesh->mp_index_buffer[0][1];
			for( uint32 i = 2; i < p_mesh->m_num_indices[0]; ++i )
			{
				// Wrap the indices round.
				index0 = index1;
				index1 = index2;
				index2 = p_mesh->mp_index_buffer[0][i];

				if(( index0 != index1 ) && ( index0 != index2 ) && ( index1 != index2 ))
				{
					++valid_tris;

					D3DXVECTOR3 *p_vert0 = (D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index0 ));
					D3DXVECTOR3 *p_vert1 = (D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index1 ));
					D3DXVECTOR3 *p_vert2 = (D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index2 ));
					
					// Push this tri onto the stack.
					triSubdivideStack.Push( p_vert0 );
					triSubdivideStack.Push( p_vert1 );
					triSubdivideStack.Push( p_vert2 );

					// Figure the area of this tri.
					Mth::Vector p(	p_vert1->x - p_vert0->x, p_vert1->y - p_vert0->y, p_vert1->z - p_vert0->z );
					Mth::Vector q(	p_vert2->x - p_vert0->x, p_vert2->y - p_vert0->y, p_vert2->z - p_vert0->z );
					Mth::Vector r(( p[Y] * q[Z] ) - ( q[Y] * p[Z] ), ( p[Z] * q[X] ) - ( q[Z] * p[X] ), ( p[X] * q[Y] ) - ( q[X] * p[Y] ));
					float area_squared = r.LengthSqr();
					if( area_squared > shatterAreaTest )
					{
						// We will need to subdivide - each subdivision will result in an area one quarter the previous area
						// (and thusly the square of the area will be one sixteenth the previous area).
						int num_extra_tris = 1;
						while( area_squared > shatterAreaTest )
						{
							num_extra_tris *= 4;
							area_squared *= ( 1.0f / 16.0f );
						}
					
						// This original tri will not be added...
						--valid_tris;

						// ...however, the subdivided versions will.
						valid_tris += num_extra_tris;
					}
				}
			}

			if( valid_tris == 0 )
			{
				continue;
			}
			
			// Create a tracking structure for this mesh.
			sXboxShatterInstanceDetails *p_details		= new sXboxShatterInstanceDetails( valid_tris, p_mesh );
			uint8						*p_write_vertex	= p_details->mp_vertex_buffer;
			uint32						details_index	= 0;

			Mth::Vector					spread_center	= shatterVelocity * -shatterSpreadFactor;
			float						base_speed		= shatterVelocity.Length();

			spread_center += Mth::Vector( p_mesh->m_sphere_center.x, p_mesh->m_sphere_center.y, p_mesh->m_sphere_center.z );
			
			// Add the tracking structure to the table.
			p_shatter_details_table->PutItem((uint32)p_details, p_details );
			
			// Process-subdivide the entire stack.
			uint8 *p_copy_vertex = p_write_vertex;
			while( subdivide_tri_stack( &p_write_vertex, p_mesh ));
					
			// Copy the (possibly subdivided) vertex data over.
			Dbg_Assert(((uint32)p_write_vertex - (uint32)p_copy_vertex ) <= ( valid_tris * 3 * p_mesh->m_vertex_stride ));
			while( p_copy_vertex < p_write_vertex )
			{
				Dbg_Assert( details_index < valid_tris );
				
				D3DXVECTOR3 *p_vert0 = (D3DXVECTOR3*)( p_copy_vertex + ( p_mesh->m_vertex_stride * 0 ));
				D3DXVECTOR3 *p_vert1 = (D3DXVECTOR3*)( p_copy_vertex + ( p_mesh->m_vertex_stride * 1 ));
				D3DXVECTOR3 *p_vert2 = (D3DXVECTOR3*)( p_copy_vertex + ( p_mesh->m_vertex_stride * 2 ));

				// Calculate position as the midpoint of the three vertices per poly.
				p_details->mp_positions[details_index][X] = ( p_vert0->x + p_vert1->x + p_vert2->x ) * ( 1.0f / 3.0f );
				p_details->mp_positions[details_index][Y] = ( p_vert0->y + p_vert1->y + p_vert2->y ) * ( 1.0f / 3.0f );
				p_details->mp_positions[details_index][Z] = ( p_vert0->z + p_vert1->z + p_vert2->z ) * ( 1.0f / 3.0f );

				// Calculate the vector <velocity> back from the bounding box of the object. Then use this to figure the 'spread' of the
				// shards by calculating the vector from this position to the center of each shard.
				float speed = base_speed + ( base_speed * (( shatterVelocityVariance * rand() ) / RAND_MAX ));
				p_details->mp_velocities[details_index] = ( p_details->mp_positions[details_index] - spread_center ).Normalize( speed );

				Mth::Vector axis( -1.0f + ( 2.0f * (float)rand() / RAND_MAX ), -1.0f + ( 2.0f * (float)rand() / RAND_MAX ), -1.0f + ( 2.0f * (float)rand() / RAND_MAX ));
				axis.Normalize();
				p_details->mp_matrices[details_index].Ident();
				p_details->mp_matrices[details_index].Rotate( axis, 0.1f * ((float)rand() / RAND_MAX ));

				p_copy_vertex += ( p_mesh->m_vertex_stride * 3 );
						
				++details_index;
			}
		}
	}
}



/******************************************************************************
 *
 * 
 *****************************************************************************/
void plat_shatter_update( sShatterInstanceDetails *p_details, float framelength )
{
	sXboxShatterInstanceDetails *p_xbox_details = static_cast<sXboxShatterInstanceDetails*>( p_details );
	
	BYTE *p_vert_data = p_xbox_details->mp_vertex_buffer;
	
	// Load up initial three vertex pointers.
	D3DXVECTOR3 *p_v0	= (D3DXVECTOR3*)( p_vert_data );
	D3DXVECTOR3 *p_v1	= (D3DXVECTOR3*)( p_vert_data + p_xbox_details->mp_mesh->m_vertex_stride );
	D3DXVECTOR3 *p_v2	= (D3DXVECTOR3*)( p_vert_data + ( 2 * p_xbox_details->mp_mesh->m_vertex_stride ));
	
	for( int i = 0; i < p_details->m_num_triangles; ++i )
	{
		// To move the shatter pieces:
		// 1) subtract position from each vertex
		// 2) rotate
		// 3) update position with velocity
		// 4) add new position to each vertex

		// The matrix holds 3 vectors at once.
		Mth::Matrix m;
		m[X].Set( p_v0->x - p_details->mp_positions[i][X], p_v0->y - p_details->mp_positions[i][Y], p_v0->z - p_details->mp_positions[i][Z] );
		m[Y].Set( p_v1->x - p_details->mp_positions[i][X], p_v1->y - p_details->mp_positions[i][Y], p_v1->z - p_details->mp_positions[i][Z] );
		m[Z].Set( p_v2->x - p_details->mp_positions[i][X], p_v2->y - p_details->mp_positions[i][Y], p_v2->z - p_details->mp_positions[i][Z] );
         
		m[X].Rotate( p_details->mp_matrices[i] );
		m[Y].Rotate( p_details->mp_matrices[i] );
		m[Z].Rotate( p_details->mp_matrices[i] );

		// Update the position and velocity of the shatter piece, dealing with bouncing if necessary.
		p_details->UpdateParameters( i, framelength );
      
		m[X] += p_details->mp_positions[i]; 
		m[Y] += p_details->mp_positions[i]; 
		m[Z] += p_details->mp_positions[i];

		p_v0->x = m[X][X]; p_v0->y = m[X][Y]; p_v0->z = m[X][Z];
		p_v1->x = m[Y][X]; p_v1->y = m[Y][Y]; p_v1->z = m[Y][Z];
		p_v2->x = m[Z][X]; p_v2->y = m[Z][Y]; p_v2->z = m[Z][Z];

		p_v0 = (D3DXVECTOR3*)(((BYTE*)p_v0 ) + ( p_xbox_details->mp_mesh->m_vertex_stride * 3 ));
		p_v1 = (D3DXVECTOR3*)(((BYTE*)p_v1 ) + ( p_xbox_details->mp_mesh->m_vertex_stride * 3 ));
		p_v2 = (D3DXVECTOR3*)(((BYTE*)p_v2 ) + ( p_xbox_details->mp_mesh->m_vertex_stride * 3 ));
	}

	// Also process normals if they exist.
	if( p_xbox_details->mp_mesh->m_normal_offset > 0 )
	{
		p_v0	= (D3DXVECTOR3*)( p_vert_data + p_xbox_details->mp_mesh->m_normal_offset );
		p_v1	= (D3DXVECTOR3*)( p_vert_data + p_xbox_details->mp_mesh->m_normal_offset + p_xbox_details->mp_mesh->m_vertex_stride );
		p_v2	= (D3DXVECTOR3*)( p_vert_data + p_xbox_details->mp_mesh->m_normal_offset + ( 2 * p_xbox_details->mp_mesh->m_vertex_stride ));
	
		for( int i = 0; i < p_details->m_num_triangles; ++i )
		{
			// The matrix holds 3 vectors at once.
			Mth::Matrix m;
			m[X].Set( p_v0->x, p_v0->y, p_v0->z );
			m[Y].Set( p_v1->x, p_v1->y, p_v1->z );
			m[Z].Set( p_v2->x, p_v2->y, p_v2->z );
         
			m[X].Rotate( p_details->mp_matrices[i] );
			m[Y].Rotate( p_details->mp_matrices[i] );
			m[Z].Rotate( p_details->mp_matrices[i] );

			p_v0->x = m[X][X]; p_v0->y = m[X][Y]; p_v0->z = m[X][Z];
			p_v1->x = m[Y][X]; p_v1->y = m[Y][Y]; p_v1->z = m[Y][Z];
			p_v2->x = m[Z][X]; p_v2->y = m[Z][Y]; p_v2->z = m[Z][Z];

			p_v0 = (D3DXVECTOR3*)(((BYTE*)p_v0 ) + ( p_xbox_details->mp_mesh->m_vertex_stride * 3 ));
			p_v1 = (D3DXVECTOR3*)(((BYTE*)p_v1 ) + ( p_xbox_details->mp_mesh->m_vertex_stride * 3 ));
			p_v2 = (D3DXVECTOR3*)(((BYTE*)p_v2 ) + ( p_xbox_details->mp_mesh->m_vertex_stride * 3 ));
		}
	}
}



/******************************************************************************
 *
 * 
 *****************************************************************************/
void plat_shatter_render( sShatterInstanceDetails *p_details )
{
	sXboxShatterInstanceDetails *p_xbox_details = static_cast<sXboxShatterInstanceDetails*>( p_details );

	p_xbox_details->mp_mesh->mp_material->Submit();

	NxXbox::set_pixel_shader( p_xbox_details->mp_mesh->m_pixel_shader );
	NxXbox::set_vertex_shader( p_xbox_details->mp_mesh->m_vertex_shader[0] );

	NxXbox::EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_TRIANGLELIST, p_xbox_details->m_num_triangles, p_xbox_details->mp_vertex_buffer, p_xbox_details->mp_mesh->m_vertex_stride );
}

	
///////////////////////////////////////////////////////////////////
//
// FOG
//
///////////////////////////////////////////////////////////////////

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CFog::s_plat_enable_fog( bool enable )
{
	if( enable != (bool)NxXbox::EngineGlobals.fog_enabled )
	{
		NxXbox::EngineGlobals.fog_enabled = enable;
		D3DDevice_SetRenderState( D3DRS_FOGENABLE, NxXbox::EngineGlobals.fog_enabled );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CFog::s_plat_set_fog_near_distance( float distance )
{
	NxXbox::EngineGlobals.fog_start	= -distance;

	// Test code for now.
	NxXbox::EngineGlobals.fog_end	= NxXbox::EngineGlobals.fog_start - FEET_TO_INCHES( 600.0f );

	D3DDevice_SetRenderState( D3DRS_FOGSTART,	*((DWORD*)( &NxXbox::EngineGlobals.fog_start )));
	D3DDevice_SetRenderState( D3DRS_FOGEND,		*((DWORD*)( &NxXbox::EngineGlobals.fog_end )));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CFog::s_plat_set_fog_exponent( float exponent )
{
	// This is no longer a valid call.
//	if( exponent > 0.0f )
//	{
//		s_plat_enable_fog( true );
//
//		NxXbox::EngineGlobals.fog_start				= FEET_TO_INCHES( -20.0f );
//		NxXbox::EngineGlobals.fog_end				= FEET_TO_INCHES( -60.0f );
//		D3DDevice_SetRenderState( D3DRS_FOGSTART,	*((DWORD*)( &NxXbox::EngineGlobals.fog_start )));
//		D3DDevice_SetRenderState( D3DRS_FOGEND,		*((DWORD*)( &NxXbox::EngineGlobals.fog_end )));
//	}
//	else
//	{
//		s_plat_enable_fog( false );
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CFog::s_plat_set_fog_color( void )
{
}
	


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CFog::s_plat_fog_update( void )
{
}
	


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CFog::s_plat_set_fog_rgba( Image::RGBA rgba )
{
	// Alpha effectively determines the fog density, with zero alpha meaning no fog.
	if( rgba.a == 0 )
	{
		s_plat_enable_fog( false );
	}
	else
	{
		s_plat_enable_fog( true );
	}

	// Calculate alpha (and clamp between 0.0 and 1.0).
	float f_alpha = (float)rgba.a / 128.0f;
	f_alpha = Mth::Min( f_alpha, 1.0f );
	f_alpha = Mth::Max( f_alpha, 0.0f );

	// Set the density register in the pixel shader constants (uses c4.r/g/b).
	NxXbox::EngineGlobals.pixel_shader_constants[16] = f_alpha;
	NxXbox::EngineGlobals.pixel_shader_constants[17] = f_alpha;
	NxXbox::EngineGlobals.pixel_shader_constants[18] = f_alpha;

	NxXbox::EngineGlobals.fog_color	= ((uint32)rgba.r << 16 ) | ((uint32)rgba.g << 8 ) | ((uint32)rgba.b );
	D3DDevice_SetRenderState( D3DRS_FOGCOLOR, NxXbox::EngineGlobals.fog_color );
}

} // Nx
