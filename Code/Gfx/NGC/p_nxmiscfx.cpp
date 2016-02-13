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

#include <gfx/Ngc/p_nxgeom.h>
#include <gfx/Ngc/p_nxtexture.h>

#include <gfx/Ngc/nx/render.h>

#include <sys/ngc/p_prim.h>
#include <sys/ngc/p_gx.h>
#include "dolphin/base/ppcwgpipe.h"
#include "dolphin/gx/gxvert.h"

namespace Nx
{

#define	DRAW_DEBUG_LINES		0

#	if DRAW_DEBUG_LINES
static float debugpos[10240*6];
static GXColor debugcol[10240];
static int nLines = 0;
#	endif		// DRAW_DEBUG_LINES

#define ADD_LINE(_s,_e,_r,_g,_b) {	debugpos[(nLines*6)+0] = _s[X];	\
									debugpos[(nLines*6)+1] = _s[Y];    \
									debugpos[(nLines*6)+2] = _s[Z];    \
									debugpos[(nLines*6)+3] = _e[X];    \
									debugpos[(nLines*6)+4] = _e[Y];    \
									debugpos[(nLines*6)+5] = _e[Z];    \
									debugcol[nLines] = (GXColor){_r,_g,_b,255};			\
									nLines++; }							\

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sNgcScreenFlashVert
{
	float		x, y, z;
	GXColor		col;
	float		u, v;
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sNgcVert
{
	Mth::Vector		pos;
	GXColor			col;
	s16				u, v;
};


	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#if 0
struct sNgcSplatInstanceDetails : public sSplatInstanceDetails
{
	// Platform specific part.
	NxNgc::CInstance	*mp_instance;
//	NxNgc::sMaterialHeader		m_mat;
//	NxNgc::sMaterialPassHeader	m_pass;
	NxNgc::sTexture *	mp_texture;
	sNgcVert			m_verts[SPLAT_POLYS_PER_MESH * 3];
};
#endif		// 0


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sNgcShatterInstanceDetails : public sShatterInstanceDetails
{
	// Platform specific part.

						sNgcShatterInstanceDetails( int num_tris, NxNgc::sMesh *p_mesh, NxNgc::sScene *p_scene, GXVtxFmt format );
						~sNgcShatterInstanceDetails( void );
	
	NxNgc::sMesh		*mp_mesh;
	NxNgc::sScene		*mp_scene;
	sNgcVert			*mp_vertex_buffer;
	GXVtxFmt			m_vertex_format;
};


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sNgcShatterInstanceDetails::sNgcShatterInstanceDetails( int num_tris, NxNgc::sMesh *p_mesh, NxNgc::sScene *p_scene, GXVtxFmt format ) : sShatterInstanceDetails( num_tris )
{
	mp_mesh				= p_mesh;
	mp_scene			= p_scene;
	mp_vertex_buffer	= new sNgcVert[num_tris * 3];
	m_vertex_format		= format;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sNgcShatterInstanceDetails::~sNgcShatterInstanceDetails( void )
{
	delete [] mp_vertex_buffer;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#if 0
sNgcSplatInstanceDetails * getDetailsForTextureSplat( NxNgc::sTexture *p_texture )
{

	sNgcSplatInstanceDetails *p_Ngc_details;

	Dbg_Assert( p_splat_details_table );
	
	// Check to see whether we have a scene already created for this type of texture splat.
	p_splat_details_table->IterateStart();
	sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();
	while( p_details )
	{
		p_Ngc_details					= static_cast<sNgcSplatInstanceDetails*>( p_details );
//		if( p_Ngc_details->m_pass.m_texture.p_data == p_texture )
		if( p_Ngc_details->mp_texture == p_texture )
		{
			// This scene contains a material with the required texture, so use this scene.
			return p_Ngc_details;
		}
		p_details = p_splat_details_table->IterateNext();
	}
	
	// Create an (opaque) material used to render the mesh.
	p_Ngc_details = new sNgcSplatInstanceDetails;

	p_Ngc_details->mp_texture				= p_texture;

	p_Ngc_details->m_highest_active_splat	= 0;
	memset( p_Ngc_details->m_lifetimes, 0, sizeof( int ) * SPLAT_POLYS_PER_MESH );

	for( int v = 0; v < SPLAT_POLYS_PER_MESH * 3; ++v )
	{
		p_Ngc_details->m_verts[v].pos = Mth::Vector(0.0f, 0.0f, 0.0f, 1.0f);
		p_Ngc_details->m_verts[v].col = (GXColor){ 0x80, 0x80, 0x80, 0xff };
	}
	
	p_splat_details_table->PutItem((uint32)p_Ngc_details, p_Ngc_details );

	return p_Ngc_details;
}
#endif		// 0



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool subdivide_tri_stack( sNgcVert **p_write, NxNgc::sMesh *p_mesh )
{
	// If there are elements on the stack, pop off the top three vertices and subdivide if necessary.
	if( triSubdivideStack.IsEmpty())
	{
		return false;
	}

	// Three temporary buffers.
	sNgcVert v0;
	sNgcVert v1;
	sNgcVert v2;
	
	sNgcVert	*p_v0 = &v0;
	sNgcVert	*p_v1 = &v1;
	sNgcVert	*p_v2 = &v2;
	
	// Stack is LIFO, so Pop() off in reverse order.
	triSubdivideStack.Pop( p_v2 );
	triSubdivideStack.Pop( p_v1 );
	triSubdivideStack.Pop( p_v0 );
	
	// Calculate the area of this tri.
	Mth::Vector p(	p_v1->pos[X] - p_v0->pos[X], p_v1->pos[Y] - p_v0->pos[Y], p_v1->pos[Z] - p_v0->pos[Z] );
	Mth::Vector q(	p_v2->pos[X] - p_v0->pos[X], p_v2->pos[Y] - p_v0->pos[Y], p_v2->pos[Z] - p_v0->pos[Z] );
	Mth::Vector r(( p[Y] * q[Z] ) - ( q[Y] * p[Z] ), ( p[Z] * q[X] ) - ( q[Z] * p[X] ), ( p[X] * q[Y] ) - ( q[X] * p[Y] ));
	float area_squared = r.LengthSqr();

	if( area_squared > shatterAreaTest )
	{
		// Three temporary buffers.
		sNgcVert i01 = v0;
		sNgcVert i12 = v0;
		sNgcVert i20 = v0;

		// Deal with positions (always present).
		i01.pos[X] = p_v0->pos[X] + (( p_v1->pos[X] - p_v0->pos[X] ) * 0.5f );
		i01.pos[Y] = p_v0->pos[Y] + (( p_v1->pos[Y] - p_v0->pos[Y] ) * 0.5f );
		i01.pos[Z] = p_v0->pos[Z] + (( p_v1->pos[Z] - p_v0->pos[Z] ) * 0.5f );
		      
		i12.pos[X] = p_v1->pos[X] + (( p_v2->pos[X] - p_v1->pos[X] ) * 0.5f );
		i12.pos[Y] = p_v1->pos[Y] + (( p_v2->pos[Y] - p_v1->pos[Y] ) * 0.5f );
		i12.pos[Z] = p_v1->pos[Z] + (( p_v2->pos[Z] - p_v1->pos[Z] ) * 0.5f );
		      
		i20.pos[X] = p_v2->pos[X] + (( p_v0->pos[X] - p_v2->pos[X] ) * 0.5f );
		i20.pos[Y] = p_v2->pos[Y] + (( p_v0->pos[Y] - p_v2->pos[Y] ) * 0.5f );
		i20.pos[Z] = p_v2->pos[Z] + (( p_v0->pos[Z] - p_v2->pos[Z] ) * 0.5f );

		// Deal with colors (not always present).
//		if ( p_mesh->mp_colBuffer )
		{
			i01.col.r		= v0.col.r + (u8)(( v1.col.r - v0.col.r ) >> 1 );
			i12.col.r		= v1.col.r + (u8)(( v2.col.r - v1.col.r ) >> 1 );
			i20.col.r		= v2.col.r + (u8)(( v0.col.r - v2.col.r ) >> 1 );
									   						
			i01.col.g		= v0.col.g + (u8)(( v1.col.g - v0.col.g ) >> 1 );
			i12.col.g		= v1.col.g + (u8)(( v2.col.g - v1.col.g ) >> 1 );
			i20.col.g		= v2.col.g + (u8)(( v0.col.g - v2.col.g ) >> 1 );
									   						
			i01.col.b		= v0.col.b + (u8)(( v1.col.b - v0.col.b ) >> 1 );
			i12.col.b		= v1.col.b + (u8)(( v2.col.b - v1.col.b ) >> 1 );
			i20.col.b		= v2.col.b + (u8)(( v0.col.b - v2.col.b ) >> 1 );
									   						
			i01.col.a		= v0.col.a + (u8)(( v1.col.a - v0.col.a ) >> 1 );
			i12.col.a		= v1.col.a + (u8)(( v2.col.a - v1.col.a ) >> 1 );
			i20.col.a		= v2.col.a + (u8)(( v0.col.a - v2.col.a ) >> 1 );
		}

		// Deal with uv0 (not always present).
//		if ( p_mesh->mp_uvBuffer )
		{
			i01.u		= v0.u + (( v1.u - v0.u ) >> 1 );
			i01.v		= v0.v + (( v1.v - v0.v ) >> 1 );
			i12.u		= v1.u + (( v2.u - v1.u ) >> 1 );
			i12.v		= v1.v + (( v2.v - v1.v ) >> 1 );
			i20.u		= v2.u + (( v0.u - v2.u ) >> 1 );
			i20.v		= v2.v + (( v0.v - v2.v ) >> 1 );
		}
		
		// Push the four new tris onto the stack.
		triSubdivideStack.Push( &v0 );
		triSubdivideStack.Push( &i01 );
		triSubdivideStack.Push( &i20 );

		triSubdivideStack.Push( &i01 );
		triSubdivideStack.Push( &v1 );
		triSubdivideStack.Push( &i12 );

		triSubdivideStack.Push( &i01 );
		triSubdivideStack.Push( &i12 );
		triSubdivideStack.Push( &i20 );

		triSubdivideStack.Push( &i20 );
		triSubdivideStack.Push( &i12 );
		triSubdivideStack.Push( &v2 );
	}
	else
	{
		// Just copy the tri into the next available slot.
		**p_write = v0;
		(*p_write)++;
		**p_write = v1;
		(*p_write)++;
		**p_write = v2;
		(*p_write)++;
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static bool same_side( Mth::Vector &p1, Mth::Vector &p2, Mth::Vector &a, Mth::Vector &b )
{
	Mth::Vector cp1 = Mth::CrossProduct( b - a, p1 - a );
	Mth::Vector cp2 = Mth::CrossProduct( b - a, p2 - a );
	if( Mth::DotProduct( cp1, cp2 ) >= 0.0f )
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
	if( same_side( p, a, b, c ) && same_side( p, b, a, c ) && same_side( p, c, a, b ))
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
	float den = (( y4 - y3 ) * ( x2 - x1 )) - (( x4 - x3 ) * ( y2 - y1 ));

	if( den == 0.0f )
	{
		// Parallel lines.
		return false;
	}
	
	float num_a = (( x4 - x3 ) * ( y1 - y3 )) - (( y4 - y3 ) * ( x1 - x3 ));
	float num_b = (( x2 - x1 ) * ( y1 - y3 )) - (( y2 - y1 ) * ( x1 - x3 ));

	num_a /= den;
	num_b /= den;

	if(( num_a <= 1.0f ) && ( num_b <= 1.0f ))
		return true;

	return false;
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
	
	// Check that at least one corner of the texture falls within the tri.
	Mth::Vector texture_square[4] = {	Mth::Vector( -1.0f, -1.0f, 0.0f, 0.0f ),
										Mth::Vector(  1.0f, -1.0f, 0.0f, 0.0f ),
										Mth::Vector(  1.0f,  1.0f, 0.0f, 0.0f ),
										Mth::Vector( -1.0f,  1.0f, 0.0f, 0.0f )};

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

	// No corners of the texture fall within the tri. There are 3 possible explanations:
	// 1) The tri intersects the texture, but does not contain a texture corner.
	// 2) The texture lies entirely outside of the tri.
	// 3) The tri falls entirely within the texture.
	// Given the relatively small size of the textures, case (3) is extremely unlikely.

	// Perform a trivial check to see whether a corner of the tri lies within the texture. This will catch (3) and sometimes (1).
	if(( u0 >= -1.0f ) && ( u0 <= 1.0f ) && ( v0 >= -1.0f ) && ( v0 <= 1.0f ))
		return true;
	if(( u1 >= -1.0f ) && ( u1 <= 1.0f ) && ( v1 >= -1.0f ) && ( v1 <= 1.0f ))
		return true;
	if(( u2 >= -1.0f ) && ( u2 <= 1.0f ) && ( v2 >= -1.0f ) && ( v2 <= 1.0f ))
		return true;

	// Perform the complete check to see if any line segment forming the tri intersects any line segment forming the texture.
	for( int p = 0; p < 4; ++p )
	{
		int q = ( p + 1 ) % 4;
		if( line_segment_intersection( u0, v0, u1, v1, texture_square[p][X], texture_square[p][Y], texture_square[q][X], texture_square[q][Y] ))
			return true;
	}
	
	return false;
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
	// Create an (opaque) material used to render the mesh.
	NxNgc::sMaterialHeader		mat;
	NxNgc::sMaterialPassHeader	pass;

	// Header.
	mat.m_checksum			= 0xa9db601e;   // particle 
	mat.m_passes			= 1;
	mat.m_alpha_cutoff		= 1;
	mat.m_flags				= (1<<1);		// 2 sided.
//	mat.m_shininess			= 0.0f;

	// Pass 0.
	pass.m_texture.p_data	= NULL;
	pass.m_flags			= (1<<5) | (1<<6);		// clamped.
	pass.m_filter			= 0;
	pass.m_blend_mode		= (unsigned char)NxNgc::vBLEND_MODE_BLEND;
	pass.m_alpha_fix		= (unsigned char)0; 
	pass.m_k				= 0;
	pass.m_color.r			= 128;
	pass.m_color.g			= 128;
	pass.m_color.b			= 128;
	pass.m_color.a			= 255;

	// Get viewport details.
	CViewport *p_vp = CViewportManager::sGetActiveViewport( p_details->m_viewport );

	sNgcScreenFlashVert verts[4];

	verts[0].x	= p_vp->GetOriginX() * 640;
	verts[0].y	= p_vp->GetOriginY() * 448;
	verts[0].z	= p_details->m_z;
	
	verts[1].x	= verts[0].x + ( p_vp->GetWidth() * 640 );
	verts[1].y	= verts[0].y;
	verts[1].z	= verts[0].z;

	verts[2].x	= verts[0].x + ( p_vp->GetWidth() * 640 );
	verts[2].y	= verts[0].y + ( p_vp->GetHeight() * 448 );
	verts[2].z	= verts[0].z;

	verts[3].x	= verts[0].x;
	verts[3].y	= verts[0].y + ( p_vp->GetHeight() * 448 );
	verts[3].z	= verts[0].z;

	for( int v = 0; v < 4; ++v )
	{
		verts[v].col = (GXColor){ p_details->m_current.r, p_details->m_current.g, p_details->m_current.b, p_details->m_current.a };
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

		Nx::CNgcTexture *p_Ngc_texture = static_cast<CNgcTexture*>( p_details->mp_texture );
		pass.m_texture.p_data = p_Ngc_texture->GetEngineTexture();
		pass.m_flags |= (1<<0);
	}
	GX::SetZMode ( GX_FALSE, GX_ALWAYS, GX_FALSE );

	NxNgc::multi_mesh( &mat, &pass, true, false );
	GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
	GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

	// Render the triangles
	if( p_details->mp_texture )
	{
		GX::SetVtxDesc( 3, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );
	}
	else
	{
		GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT );
	}
	GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
	for (int i = 0; i < 4; i++ )
	{
		GX::Position3f32( verts[i].x, verts[i].y, verts[i].z );
		GX::Color1u32(*((u32*)&(verts[i].col)));
		if( p_details->mp_texture ) GX::TexCoord2f32( verts[i].u, verts[i].v );
	}
	GX::End();
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
#if 0
	sNgcSplatInstanceDetails *p_Ngc_details;

	Dbg_Assert( p_splat_details_table );
	
	p_splat_details_table->IterateStart();
	sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();
	while( p_details )
	{
		p_Ngc_details = static_cast<sNgcSplatInstanceDetails*>( p_details );

		//delete p_Ngc_details->mp_material;
		
		p_details = p_splat_details_table->IterateNext();

		p_splat_details_table->FlushItem((uint32)p_Ngc_details );
		delete p_Ngc_details;
	}
#endif		// 0
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_texture_splat_reset_poly( sSplatInstanceDetails *p_details, int index )
{
#if 0
	// Cast the details to Ngc details.
	sNgcSplatInstanceDetails *p_Ngc_details = static_cast<sNgcSplatInstanceDetails *>( p_details );
	
	// Force this poly to be degenerate.
	p_Ngc_details->m_verts[index * 3 + 1]	= p_Ngc_details->m_verts[index * 3];
	p_Ngc_details->m_verts[index * 3 + 2]	= p_Ngc_details->m_verts[index * 3];
#endif		// 0
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool plat_texture_splat( Nx::CSector **pp_sectors, Nx::CCollStatic **pp_collision, Mth::Vector& start, Mth::Vector& end, float size, float lifetime, Nx::CTexture *p_texture, Nx::sSplatTrailInstanceDetails *p_trail_details )
{
#if 0
	Mth::Matrix view_matrix, ortho_matrix, projection_matrix;

#	if DRAW_DEBUG_LINES
//	Gfx::AddDebugLine( start, end, MAKE_RGB( 200, 200, 0 ), MAKE_RGB( 200, 200, 0 ), 1 );
	ADD_LINE( start, end, 200, 200, 0 );
#	endif // DRAW_DEBUG_LINES
	
	// The length of the start->end line defines the view depth of the frustum.
	Mth::Vector	splat_vector	= end - start;
	float		view_depth		= splat_vector.Length();
	splat_vector.Normalize();
	
	// Calculate the parallel projection matrix. Generally the projection vector will tend to point downwards, so we use a
	// random vector in the x-z plane to define the up vector for the projection. However if this splat is part of a trail,
	// use the previous point to generate the up vector.
	if( p_trail_details )
	{
		Mth::Vector up( start[X] - p_trail_details->m_last_pos[X], start[Y] - p_trail_details->m_last_pos[Y], start[Z] - p_trail_details->m_last_pos[Z], 0.0f );

		// The height of the viewport is defined by the distance between the two points.
		float height = up.Length() * 0.5f;

		start	-= up * 0.5f;
		end		-= up * 0.5f;

		up.Normalize();

		Mth::CreateMatrixLookAt( view_matrix, start, end, up );
		Mth::CreateMatrixOrtho( ortho_matrix, size, height, 0.1f, view_depth );
	}
	else if( fabsf( splat_vector[Y] ) > 0.5f )
	{
		float angle = ((float)rand() * 2.0f * Mth::PI ) / (float)RAND_MAX;
		Mth::CreateMatrixLookAt( view_matrix, start, end, Mth::Vector( sinf( angle ), 0.0f, cosf( angle ), 0.0f ));
		Mth::CreateMatrixOrtho( ortho_matrix , size, size, 0.1f, view_depth );
	}
	else
	{
		Mth::CreateMatrixLookAt( view_matrix, start, end, Mth::Vector( 0.0f, 1.0f, 0.0f, 0.0f ));
		Mth::CreateMatrixOrtho( ortho_matrix , size, size, 0.1f, view_depth );
	}

	projection_matrix = view_matrix * ortho_matrix;

	// Pointer to the mesh we will be modifying. (Don't want to set the pointer up until we know for
	// sure that we will be adding some polys).
	sNgcSplatInstanceDetails	*p_details		= NULL;
	sNgcVert					*p_target_verts	= NULL;

	Nx::CSector *p_sector;
	while( (p_sector = *pp_sectors) )
	{
		Nx::CNgcGeom *p_Ngc_geom = static_cast<Nx::CNgcGeom*>( p_sector->GetGeom());

		if( p_Ngc_geom )
		{
#			if DRAW_DEBUG_LINES
			Mth::Vector min = p_Ngc_geom->GetBoundingBox().GetMin();
			Mth::Vector max = p_Ngc_geom->GetBoundingBox().GetMax();

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
//				Gfx::AddDebugLine( box[0], box[i], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
				ADD_LINE( box[0], box[i], 200, 0, 0 );
			}
			for ( int i = 5; i < 8; i++ )
			{
//				Gfx::AddDebugLine( box[4], box[i], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
				ADD_LINE( box[4], box[i], 200, 0, 0 );
			}
//			Gfx::AddDebugLine( box[1], box[6], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
//			Gfx::AddDebugLine( box[1], box[7], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
//			Gfx::AddDebugLine( box[2], box[5], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
//			Gfx::AddDebugLine( box[2], box[7], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
//			Gfx::AddDebugLine( box[3], box[5], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
//			Gfx::AddDebugLine( box[3], box[6], MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
			ADD_LINE( box[1], box[6], 200, 0, 0 );
			ADD_LINE( box[1], box[7], 200, 0, 0 );
			ADD_LINE( box[2], box[5], 200, 0, 0 );
			ADD_LINE( box[2], box[7], 200, 0, 0 );
			ADD_LINE( box[3], box[5], 200, 0, 0 );
			ADD_LINE( box[3], box[6], 200, 0, 0 );
#			endif // DRAW_DEBUG_LINES
			
			// For each mesh in the geom...
			for( uint32 m = 0; m < p_Ngc_geom->m_num_mesh; ++m )
			{
				NxNgc::sMesh *p_mesh = p_Ngc_geom->m_mesh_array[m];
				
				NxNgc::sScene *p_scene = p_Ngc_geom->mp_scene->GetEngineScene();

				// Not allowed on meshes which are flagged not to shadow.
				if( p_mesh->m_flags & NxNgc::sMesh::MESH_FLAG_NO_SKATER_SHADOW )
					continue;
				
				// Count indices.
				int num_indices = 0;
				unsigned char * p_start = (unsigned char *)&p_mesh->mp_dl[1];
//				uint32 cp = (p_start[2]<<24)|(p_start[3]<<16)|(p_start[4]<<8)|p_start[5];
				unsigned char * p_end = &p_start[p_mesh->mp_dl->m_size];
				p_start = &p_start[p_mesh->mp_dl->m_index_offset];		// Skip to actual 1st GDBegin.
				unsigned char * p8 = p_start;

				int stride = p_mesh->mp_dl->m_index_stride;

				unsigned char * p_index_buffer  = &p8[1];

				while ( p8 < p_end )
				{
					if ( ( p8[0] & 0xf8 ) == GX_TRIANGLESTRIP )
					{
						// Found a triangle strip - parse it.
						int num_verts = ( p8[1] << 8 ) | p8[2];
						p8 += 3;		// Skip GDBegin

						num_indices += num_verts;

						p8 += stride * 2 * num_verts;
					}
					else
					{
						break;
					}
				}

				if ( p_index_buffer )
				{
					// Create the index buffer.
					uint polys = 0;
					uint16 idxbuf[2048*3];		// 12k.
					uint16 * p_dest = idxbuf;
					uint8 * p_source = (uint8*)p_index_buffer;
					uint16 total = 0;
					while ( total < num_indices )
					{
						uint16 num = ( p_source[0] << 8 ) | p_source[1];
						p_source += 2;
						uint16 i0;
						uint16 i1 = ( p_source[0] << 8 ) | p_source[1];
						p_source += stride * 2;
						uint16 i2 = ( p_source[0] << 8 ) | p_source[1];
						p_source += stride * 2;
						for ( int vv = 2; vv < num; vv++ )
						{
							i0 = i1;
							i1 = i2;
							i2 = ( p_source[0] << 8 ) | p_source[1];
							p_source += stride * 2;
							*p_dest++ = i0;
							*p_dest++ = i1;
							*p_dest++ = i2;
							polys++;
						}
						p_source += 1;
						total += num + 1;		// Count each index + the count.
					}

					// Now scan through each non-degenerate tri, checking the verts to see if they are within scope.
					for( uint32 i = 0; i < polys; ++i )
					{
						// Wrap the indices round.
						uint16 index0 = idxbuf[(i*3)+0];
						uint16 index1 = idxbuf[(i*3)+1];
						uint16 index2 = idxbuf[(i*3)+2];

						if(( index0 != index1 ) && ( index0 != index2 ) && ( index1 != index2 ))
						{
							Mth::Vector v0( p_scene->mp_pos_pool[(index0*3)+0],
											p_scene->mp_pos_pool[(index0*3)+1],
											p_scene->mp_pos_pool[(index0*3)+2] );
							Mth::Vector v1( p_scene->mp_pos_pool[(index1*3)+0],
											p_scene->mp_pos_pool[(index1*3)+1],
											p_scene->mp_pos_pool[(index1*3)+2] );
							Mth::Vector v2( p_scene->mp_pos_pool[(index2*3)+0],
											p_scene->mp_pos_pool[(index2*3)+1],
											p_scene->mp_pos_pool[(index2*3)+2] );

							v0[W] = v1[W] = v2[W] = 1.0f;		// Make sure they are points, not vectors

	//						OSReport( "Looking at: (%6.3f, %6.3f, %6.3f) (%6.3f, %6.3f, %6.3f) (%6.3f, %6.3f, %6.3f)\n", v0[X], v0[Y], v0[Z], v1[X], v1[Y], v1[Z], v2[X], v2[Y], v2[Z] );

							Mth::Vector uvprojections[3];
							uvprojections[0] = v0 * projection_matrix;
							uvprojections[1] = v1 * projection_matrix;
							uvprojections[2] = v2 * projection_matrix;

							// Check that they are in the frustum
//							if(( uvprojections[0][X] < -1.0f ) && ( uvprojections[1][X] < -1.0f ) && ( uvprojections[2][X] < -1.0f ))
//								continue;
//							if(( uvprojections[0][Y] < -1.0f ) && ( uvprojections[1][Y] < -1.0f ) && ( uvprojections[2][Y] < -1.0f ))
//								continue;
//							if(( uvprojections[0][X] > 1.0f ) && ( uvprojections[1][X] > 1.0f ) && ( uvprojections[2][X] > 1.0f ))
//								continue;
//							if(( uvprojections[0][Y] > 1.0f ) && ( uvprojections[1][Y] > 1.0f ) && ( uvprojections[2][Y] > 1.0f ))
//								continue;
							if(( uvprojections[0][Z] < -1.0f ) && ( uvprojections[1][Z] < -1.0f ) && ( uvprojections[2][Z] < -1.0f ))
								continue;
							if(( uvprojections[0][Z] > 1.0f ) && ( uvprojections[1][Z] > 1.0f ) && ( uvprojections[2][Z] > 1.0f ))
								continue;

							// Okay, this tri lies within the projection frustum. Now check that it intersects the texture
							if( !tri_texture_intersect( uvprojections[0][X], uvprojections[0][Y],
														uvprojections[1][X], uvprojections[1][Y],
														uvprojections[2][X], uvprojections[2][Y] ))
							{
								continue;
							}

//#ifdef SHORT_VERT
							Mth::Vector	s = v1 - v0;
							Mth::Vector	t = v2 - v0;
							Mth::Vector normal = Mth::CrossProduct( s, t );
							normal.Normalize();

							v0 -= normal;
							v1 -= normal;
							v2 -= normal;
//#endif		// SHORT_VERT 

	//						OSReport( "Added: (%5.1f, %5.1f, %5.1f) (%5.1f, %5.1f, %5.1f) (%5.1f, %5.1f, %5.1f)\n", v0[X], v0[Y], v0[Z], v1[X], v1[Y], v1[Z], v2[X], v2[Y], v2[Z] );

							// Okay, this tri lies within the projection frustum. Get a pointer to the mesh used for rendering texture splats
							// with the given texture. (Note this will create a new instance to handle texture splats of this texture if one
							// does not already exist).
							if( p_target_verts == NULL )
							{
								CNgcTexture *p_Ngc_texture	= static_cast<CNgcTexture*>( p_texture );
								p_details						= getDetailsForTextureSplat( p_Ngc_texture->GetEngineTexture());
								p_target_verts					= p_details->m_verts;
								Dbg_Assert( p_target_verts );
							}

							// If we have trails, scale up the mapping by one pixel in all directions.
							// (effectively 2 pixels in u and 2 pixels in v ).
							float up_one_u_pixel;
							float up_one_v_pixel;
							if( p_trail_details )
							{
								up_one_u_pixel = 1.0f - ( 1.0f / ( p_texture->GetWidth() / 2.0f ) );
								up_one_v_pixel = 1.0f - ( 1.0f / ( p_texture->GetHeight() / 2.0f ) );
							}
							else
							{
								up_one_u_pixel = 1.0f;
								up_one_v_pixel = 1.0f;
							}

							// Scan through the lifetimes, finding a 'dead' poly (lifetime == 0), or the oldest.
							uint32 idx						= p_details->GetOldestSplat();

							// Convert lifetime from seconds to milliseconds.
							p_details->m_lifetimes[idx]		= (int)( lifetime * 1000.0f );

							// Set up the corresponding vertices. First write the positions.
							uint32 index					= idx * 3;
							p_target_verts[index + 0].pos	= v0;
							p_target_verts[index + 1].pos	= v1;
							p_target_verts[index + 2].pos	= v2;

							// Then the uv's.
							p_target_verts[index + 0].u		= (s16)( ( ( ( uvprojections[0][X] * 0.5f ) + 0.5f ) * up_one_u_pixel ) * 256.0f );
							p_target_verts[index + 0].v		= (s16)( ( ( ( uvprojections[0][Y] * 0.5f ) + 0.5f ) * up_one_v_pixel ) * 256.0f );
							p_target_verts[index + 1].u		= (s16)( ( ( ( uvprojections[1][X] * 0.5f ) + 0.5f ) * up_one_u_pixel ) * 256.0f );
							p_target_verts[index + 1].v		= (s16)( ( ( ( uvprojections[1][Y] * 0.5f ) + 0.5f ) * up_one_v_pixel ) * 256.0f );
							p_target_verts[index + 2].u		= (s16)( ( ( ( uvprojections[2][X] * 0.5f ) + 0.5f ) * up_one_u_pixel ) * 256.0f );
							p_target_verts[index + 2].v		= (s16)( ( ( ( uvprojections[2][Y] * 0.5f ) + 0.5f ) * up_one_v_pixel ) * 256.0f );

							// Now the colors
							p_target_verts[index + 0].col = (GXColor){128,128,128,255};		// p_coll_geom->GetVertexRGBA(p_coll_geom->GetFaceVertIndex(*p_face_indexes, 0));
							p_target_verts[index + 1].col = (GXColor){128,128,128,255};     // p_coll_geom->GetVertexRGBA(p_coll_geom->GetFaceVertIndex(*p_face_indexes, 1));
							p_target_verts[index + 2].col = (GXColor){128,128,128,255};     // p_coll_geom->GetVertexRGBA(p_coll_geom->GetFaceVertIndex(*p_face_indexes, 2));
//.col

							Mth::Vector	*p_v0 = &( p_target_verts[index + 0].pos );
							Mth::Vector	*p_v1 = &( p_target_verts[index + 1].pos );
							Mth::Vector	*p_v2 = &( p_target_verts[index + 2].pos );
							Mth::Vector pv(	p_v1->GetX() - p_v0->GetX(), p_v1->GetY() - p_v0->GetY(), p_v1->GetZ() - p_v0->GetZ() );
							Mth::Vector qv(	p_v2->GetX() - p_v0->GetX(), p_v2->GetY() - p_v0->GetY(), p_v2->GetZ() - p_v0->GetZ() );
							Mth::Vector r(( pv[Y] * qv[Z] ) - ( qv[Y] * pv[Z] ), ( pv[Z] * qv[X] ) - ( qv[Z] * pv[X] ), ( pv[X] * qv[Y] ) - ( qv[X] * pv[Y] ));
							float area_squared = r.LengthSqr();
				
							// Set the shatter test to ensure that we don't subdivide too far. Note that each successive subdivision will quarter
							// the area of each triangle, which means the area *squared* of each triangle will become 1/16th of the previous value.
							shatterAreaTest = area_squared / 128.0f;
				
							triSubdivideStack.Reset();
							triSubdivideStack.SetBlockSize( sizeof(sNgcVert) );
							triSubdivideStack.Push( &p_target_verts[index + 0] );
							triSubdivideStack.Push( &p_target_verts[index + 1] );
							triSubdivideStack.Push( &p_target_verts[index + 2] );
				
							// Allocate a block of memory into which the subdivision stack will write the results.
							Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
							sNgcVert		*p_array		= new sNgcVert[1024];
							sNgcVert		*p_array_start	= p_array;
							sNgcVert		*p_array_loop	= p_array;
							//memset( p_array, 0, sizeof(sNgcVert) * 1024 );
							Mem::Manager::sHandle().PopContext();
				
							while( subdivide_tri_stack( &p_array, NULL ));
				
							// Ensure we haven't overrun the buffer.
							Dbg_Assert((uint32)p_array - (uint32)p_array_loop < ( sizeof(sNgcVert) * 1024 ));
				
							float oo_up_one_u_pixel = 1.0f / up_one_u_pixel;
							float oo_up_one_v_pixel = 1.0f / up_one_v_pixel;
				
				//			int vert_idx = 0;
							bool no_polys = true;
				
							while( p_array_loop != p_array )
							{
								Dbg_Assert(((uint32) p_array_loop) < ((uint32) p_array));
				//				Dbg_Message("Looking at vert %d", vert_idx);
								// Add this triangle, *if* it is valid.
								if( tri_texture_intersect(((p_array_loop[0].u * oo_up_one_u_pixel) - 0.5f) * 2.0f,
														  ((p_array_loop[0].v * oo_up_one_v_pixel) - 0.5f) * 2.0f,
														  ((p_array_loop[1].u * oo_up_one_u_pixel) - 0.5f) * 2.0f,
														  ((p_array_loop[1].v * oo_up_one_v_pixel) - 0.5f) * 2.0f,
														  ((p_array_loop[2].u * oo_up_one_u_pixel) - 0.5f) * 2.0f,
														  ((p_array_loop[2].v * oo_up_one_v_pixel) - 0.5f) * 2.0f ))
				//				if( tri_texture_intersect((p_array_loop[0].u * 2.0f * oo_up_one_u_pixel) - 1.0f,
				//										  (p_array_loop[0].v * 2.0f * oo_up_one_v_pixel) - 1.0f,
				//										  (p_array_loop[1].u * 2.0f * oo_up_one_u_pixel) - 1.0f,
				//										  (p_array_loop[1].v * 2.0f * oo_up_one_v_pixel) - 1.0f,
				//										  (p_array_loop[2].u * 2.0f * oo_up_one_u_pixel) - 1.0f,
				//										  (p_array_loop[2].v * 2.0f * oo_up_one_v_pixel) - 1.0f ))
								{
				//					Dbg_Message("Accepted vert %d", vert_idx);
									// Convert lifetime from seconds to milliseconds.
									p_details->m_lifetimes[idx]		= (int)( lifetime * 1000.0f );
				
									p_target_verts[index + 0].pos	= p_array_loop[0].pos;
									p_target_verts[index + 1].pos	= p_array_loop[1].pos;
									p_target_verts[index + 2].pos	= p_array_loop[2].pos;
				
									p_target_verts[index + 0].u		= p_array_loop[0].u;
									p_target_verts[index + 0].v		= p_array_loop[0].v;
									p_target_verts[index + 1].u		= p_array_loop[1].u;
									p_target_verts[index + 1].v		= p_array_loop[1].v;
									p_target_verts[index + 2].u		= p_array_loop[2].u;
									p_target_verts[index + 2].v		= p_array_loop[2].v;
				
									p_target_verts[index + 0].col	= p_array_loop[0].col;
									p_target_verts[index + 1].col	= p_array_loop[1].col;
									p_target_verts[index + 2].col	= p_array_loop[2].col;
				
									idx								= p_details->GetOldestSplat();
									index							= idx * 3;
									Dbg_Assert((index + 2) < (SPLAT_POLYS_PER_MESH * 3));
									no_polys 						= false;
								}
				
								p_array_loop					+= 3;
				//				vert_idx++;
							}
				
							delete [] p_array_start;
				
							// Check if all the new polys were rejected.  Don't know why this happens, but we want
							// to get rid of the original texture.
							if (no_polys)
							{
								plat_texture_splat_reset_poly( p_details, idx );
							}
				
				

	//						OSReport( "Add U0: %6.1f %6.1f\n", p_target_verts[index + 0].u, p_target_verts[index + 0].v );
	//						OSReport( "Add U1: %6.1f %6.1f\n", p_target_verts[index + 1].u, p_target_verts[index + 1].v );
	//						OSReport( "Add U2: %6.1f %6.1f\n", p_target_verts[index + 2].u, p_target_verts[index + 2].v );

	#						if DRAW_DEBUG_LINES
	//						D3DXVECTOR3* p_d3dvert;
	//						p_d3dvert = (D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index0 ));
	//						Mth::Vector v0( p_d3dvert->x, p_d3dvert->y, p_d3dvert->z );
	//						p_d3dvert = (D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index1 ));
	//						Mth::Vector v1( p_d3dvert->x, p_d3dvert->y, p_d3dvert->z );
	//						p_d3dvert = (D3DXVECTOR3*)( p_vert_data + ( p_mesh->m_vertex_stride * index2 ));
	//						Mth::Vector v2( p_d3dvert->x, p_d3dvert->y, p_d3dvert->z );
	//						Gfx::AddDebugLine( v0, v1, MAKE_RGB( 0, 200, 200 ), MAKE_RGB( 0, 200, 200 ), 1 );
	//						Gfx::AddDebugLine( v1, v2, MAKE_RGB( 0, 200, 200 ), MAKE_RGB( 0, 200, 200 ), 1 );
	//						Gfx::AddDebugLine( v2, v0, MAKE_RGB( 0, 200, 200 ), MAKE_RGB( 0, 200, 200 ), 1 );

	//						v0[Y] += 10;
	//						v1[Y] += 10;
	//						v2[Y] += 10;

							ADD_LINE( v0, v1, 0, 200, 200 );
							ADD_LINE( v1, v2, 0, 200, 200 );
							ADD_LINE( v2, v0, 0, 200, 200 );
	#						endif // DRAW_DEBUG_LINES
						}
					}
				}
			}
		}
		++pp_sectors;
	}
#endif		// 0
	
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_texture_splat_render( void )
{
#if 0
//#	if DRAW_DEBUG_LINES
//#	endif 		//DRAW_DEBUG_LINES
//


	sNgcSplatInstanceDetails *p_Ngc_details;

	Dbg_Assert( p_splat_details_table );

	p_splat_details_table->IterateStart();
	sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();
	while( p_details )
	{
		p_Ngc_details = static_cast<sNgcSplatInstanceDetails*>( p_details );

		if( p_Ngc_details->m_highest_active_splat >= 0 )
		{
//			NxNgc::multi_mesh( &p_Ngc_details->m_mat, &p_Ngc_details->m_pass, true, true );

			GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
		
//			NxNgc::sMaterialHeader mat;
//			NxNgc::sMaterialPassHeader pass;
//		
//			mat.m_checksum			= 0;
//			mat.m_passes			= 1;
//			mat.m_alpha_cutoff		= 0;
//			mat.m_flags				= 0;
//			mat.m_material_dl_id	= 0;
//			mat.m_draw_order		= 0;
//			mat.m_pass_item			= 0;
//			mat.m_texture_dl_id		= 0;
//		
//			pass.m_texture.p_data	= p_Ngc_details->mp_texture;
//			pass.m_flags			= (1<<0)|(1<<5)|(1<<6);
//			pass.m_filter			= 0;
//			pass.m_blend_mode		= (unsigned char)NxNgc::vBLEND_MODE_BLEND;
//			pass.m_alpha_fix		= 128;
//			pass.m_uv_wibble_index	= 0;
//			pass.m_color			= (GXColor){128,128,128,255};
//			pass.m_k				= 0;
//			pass.m_u_tile			= 0;
//			pass.m_v_tile			= 0;
//		
//			multi_mesh( &mat, &pass, true, true );





//			GX::SetPointSize( 6, GX_TO_ONE );
//
//			GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
//			GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
//			GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
//			GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
//
//			GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
//												   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
//												   GX_TEV_SWAP0, GX_TEV_SWAP0 );
//
//			GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
//											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
//			GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//			GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,0,255,255} );
//
//			GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
//
//			sNgcVert *p_vert_array = p_Ngc_details->m_verts;
//			for (int i = 0; i < ( p_details->m_highest_active_splat + 1 ); i++, p_vert_array += 3)
//			{
////				printf( "Splat Tri:\n%8.3f %8.3f %8.3f\n", p_vert_array[0].pos[X], p_vert_array[0].pos[Y], p_vert_array[0].pos[Z] );
////				printf( "%8.3f %8.3f %8.3f\n", p_vert_array[1].pos[X], p_vert_array[1].pos[Y], p_vert_array[1].pos[Z] );
////				printf( "%8.3f %8.3f %8.3f\n", p_vert_array[2].pos[X], p_vert_array[2].pos[Y], p_vert_array[2].pos[Z] );
//				GX::Begin( GX_LINES, GX_VTXFMT0, 2 ); 
//				GX::Position3f32( p_vert_array[0].pos[X], p_vert_array[0].pos[Y], p_vert_array[0].pos[Z] );
//				GX::Position3f32( p_vert_array[1].pos[X], p_vert_array[1].pos[Y], p_vert_array[1].pos[Z] );
//				GX::End(); 
//				GX::Begin( GX_LINES, GX_VTXFMT0, 2 ); 
//				GX::Position3f32( p_vert_array[1].pos[X], p_vert_array[1].pos[Y], p_vert_array[1].pos[Z] );
//				GX::Position3f32( p_vert_array[2].pos[X], p_vert_array[2].pos[Y], p_vert_array[2].pos[Z] );
//				GX::End(); 
//				GX::Begin( GX_LINES, GX_VTXFMT0, 2 ); 
//				GX::Position3f32( p_vert_array[2].pos[X], p_vert_array[2].pos[Y], p_vert_array[2].pos[Z] );
//				GX::Position3f32( p_vert_array[0].pos[X], p_vert_array[0].pos[Y], p_vert_array[0].pos[Z] );
//				GX::End(); 
//			}
////			GX::End();
//
//





			GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

			GX::UploadTexture(  p_Ngc_details->mp_texture->pTexelData,
								p_Ngc_details->mp_texture->ActualWidth,
								p_Ngc_details->mp_texture->ActualHeight,
								GX_TF_CMPR,
								GX_CLAMP,
								GX_CLAMP,
								GX_FALSE,
								GX_LINEAR,
								GX_LINEAR,
								0.0f,
								0.0f,
								0.0f,
								GX_FALSE,
								GX_TRUE,
								GX_ANISO_1,
								GX_TEXMAP0 ); 
			GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, p_Ngc_details->mp_texture->ActualWidth, p_Ngc_details->mp_texture->ActualHeight );
		
			GX::SetTexChanTevIndCull( 1, 0, 1, 0, GX_CULL_NONE );
			GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
			GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
		
			GX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0 );
			GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );		// Replace
//			GX::SetBlendMode( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
		
			GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
			GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
									 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV,
									 GX_TEV_SWAP0, GX_TEV_SWAP0 );
			GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );


			f32 pm[GX_PROJECTION_SZ];
			GX::GetProjectionv( pm );
			float value = pm[6] + ( 150.0f * 5 ) * pm[5];

			GXWGFifo.u8 = GX_LOAD_XF_REG;
			GXWGFifo.u16 = 0;
			GXWGFifo.u16 = 0x1025;
			GXWGFifo.f32 = value;


//			GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
//			GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

			// Render the triangles
			GX::SetVtxDesc( 3, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );
//			GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
			GX::Begin( GX_TRIANGLES, GX_VTXFMT6, ( p_details->m_highest_active_splat + 1 ) * 3 ); 
			sNgcVert *p_vert_array = p_Ngc_details->m_verts;
			for (int i = 0; i < ( p_details->m_highest_active_splat + 1 ); i++, p_vert_array += 3)
			{
				GX::Position3f32( p_vert_array[0].pos[X], p_vert_array[0].pos[Y], p_vert_array[0].pos[Z] );
				GX::Color1u32(*((u32*)&(p_vert_array[0].col)));
				GX::TexCoord2s16( p_vert_array[0].u, p_vert_array[0].v );

				GX::Position3f32( p_vert_array[1].pos[X], p_vert_array[1].pos[Y], p_vert_array[1].pos[Z] );
				GX::Color1u32(*((u32*)&(p_vert_array[1].col)));
				GX::TexCoord2s16( p_vert_array[1].u, p_vert_array[1].v );

				GX::Position3f32( p_vert_array[2].pos[X], p_vert_array[2].pos[Y], p_vert_array[2].pos[Z] );
				GX::Color1u32(*((u32*)&(p_vert_array[2].col)));
				GX::TexCoord2s16( p_vert_array[2].u, p_vert_array[2].v );

//				GX::Position3f32( 0.0f, 0.0f, 0.0f );
//				GX::Position3f32( 0.0f, 0.0f, 0.0f );
//				GX::Position3f32( 0.0f, 0.0f, 0.0f );
//				GX::Position3f32( 0.0f, 0.0f, 0.0f );
//  			GX::Position3f32( 0.0f, 0.0f, 0.0f );
//				GX::Position3f32( 0.0f, 0.0f, 0.0f );
			}
			GX::End();
		}
		
		p_details = p_splat_details_table->IterateNext();
	}
#endif		// 0
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
	sNgcShatterInstanceDetails *p_Ngc_details;

	Dbg_Assert( p_shatter_details_table );
	
	p_shatter_details_table->IterateStart();
	sShatterInstanceDetails *p_details = p_shatter_details_table->IterateNext();
	while( p_details )
	{
		p_Ngc_details = static_cast<sNgcShatterInstanceDetails*>( p_details );
		
		p_details = p_shatter_details_table->IterateNext();

		p_shatter_details_table->FlushItem((uint32)p_Ngc_details );
		delete p_Ngc_details;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_shatter( CGeom *p_geom )
{
#if 1
	CNgcGeom *p_Ngc_geom = static_cast<CNgcGeom*>( p_geom );

	// For each mesh in the geom...
	for( uint32 m = 0; m < p_Ngc_geom->m_num_mesh; ++m )
	{
		NxNgc::sMesh *p_mesh = p_Ngc_geom->m_mesh_array[m];

		NxNgc::sScene *p_scene = p_Ngc_geom->mp_scene->GetEngineScene();

		// Count indices.
		int num_indices = 0;
		unsigned char * p_start = (unsigned char *)&p_mesh->mp_dl[1];
		uint32 cp = (p_start[2]<<24)|(p_start[3]<<16)|(p_start[4]<<8)|p_start[5];
		unsigned char * p_end = &p_start[p_mesh->mp_dl->m_size];
		p_start = &p_start[p_mesh->mp_dl->m_index_offset];		// Skip to actual 1st GDBegin.
		unsigned char * p8 = p_start;

		int stride = p_mesh->mp_dl->m_index_stride;

		unsigned char * p_index_buffer  = &p8[1];

		int off = 2 + ( ( cp & ((1<<11)|(1<<12)) ) ? 2 : 0 );

		GXVtxFmt format = GX_VTXFMT2;

		while ( p8 < p_end )
		{
			if ( ( p8[0] & 0xf8 ) == GX_TRIANGLESTRIP )
			{
				format = (GXVtxFmt)(p8[0] & 7);
				// Found a triangle strip - parse it.
				int num_verts = ( p8[1] << 8 ) | p8[2];
				p8 += 3;		// Skip GDBegin

				num_indices += num_verts;

				p8 += stride * 2 * num_verts;
			}
			else
			{
				break;
			}
		}

		if( num_indices >= 3 )
		{
			triSubdivideStack.SetBlockSize( sizeof( sNgcVert ) );

			// Create the index buffer.
			uint polys = 0;
			uint16 idxbuf[1024*3];		// 6k.
			uint16 cidxbuf[1024*3];		// 6k.
			uint16 tidxbuf[1024*3];		// 6k.
			uint16 * p_dest = idxbuf;
			uint16 * p_cdest = cidxbuf;
			uint16 * p_tdest = tidxbuf;
			uint8 * p_source = (uint8*)p_index_buffer;
			uint16 total = 0;

			while ( total < num_indices )
			{
				uint16 num = ( p_source[0] << 8 ) | p_source[1];
				p_source += 2;
				uint16 i0, ci0, ti0;
				uint16 i1 = ( p_source[0] << 8 ) | p_source[1];
				uint16 ci1 = ( p_source[off+0] << 8 ) | p_source[off+1];
				uint16 ti1 = ( p_source[off+2] << 8 ) | p_source[off+3];
				p_source += stride * 2;
				uint16 i2 = ( p_source[0] << 8 ) | p_source[1];
				uint16 ci2 = ( p_source[off+0] << 8 ) | p_source[off+1];
				uint16 ti2 = ( p_source[off+2] << 8 ) | p_source[off+3];
				p_source += stride * 2;
				for ( int vv = 2; vv < num; vv++ )
				{
					i0 = i1;
					i1 = i2;
					i2 = ( p_source[0] << 8 ) | p_source[1];
					ci0 = ci1;
					ci1 = ci2;
					ci2 = ( p_source[off+0] << 8 ) | p_source[off+1];
					ti0 = ti1;
					ti1 = ti2;
					ti2 = ( p_source[off+2] << 8 ) | p_source[off+3];
					p_source += stride * 2;
					*p_dest++ = i0;
					*p_dest++ = i1;
					*p_dest++ = i2;
					*p_cdest++ = ci0;
					*p_cdest++ = ci1;
					*p_cdest++ = ci2;
					*p_tdest++ = ti0;
					*p_tdest++ = ti1;
					*p_tdest++ = ti2;
					polys++;
				}
				p_source += 1;
				total += num + 1;		// Count each index + the count.
			}

			// First scan through each non-degenerate tri, counting them to see how many verts we'll need.
			// We also have to figure the area of the tris here, since we need to calculate the worst case given the requirements for subdivision.
			uint32 valid_tris	= 0;
			for( uint32 i = 0; i < polys; ++i )
			{
				// Wrap the indices round.
				uint16 index0 = idxbuf[(i*3)+0];
				uint16 index1 = idxbuf[(i*3)+1];
				uint16 index2 = idxbuf[(i*3)+2];

				uint16 cindex0 = cidxbuf[(i*3)+0];
				uint16 cindex1 = cidxbuf[(i*3)+1];
				uint16 cindex2 = cidxbuf[(i*3)+2];

				uint16 tindex0 = tidxbuf[(i*3)+0];
				uint16 tindex1 = tidxbuf[(i*3)+1];
				uint16 tindex2 = tidxbuf[(i*3)+2];

				if(( index0 != index1 ) && ( index0 != index2 ) && ( index1 != index2 ))
				{
					++valid_tris;

					sNgcVert v0;
					sNgcVert v1;
					sNgcVert v2;

					if ( p_mesh->mp_dl->mp_pos_pool )
					{
						v0.pos.Set( p_mesh->mp_dl->mp_pos_pool[(index0*3)+0],
									p_mesh->mp_dl->mp_pos_pool[(index0*3)+1],
									p_mesh->mp_dl->mp_pos_pool[(index0*3)+2] );
						v1.pos.Set( p_mesh->mp_dl->mp_pos_pool[(index1*3)+0],
									p_mesh->mp_dl->mp_pos_pool[(index1*3)+1],
									p_mesh->mp_dl->mp_pos_pool[(index1*3)+2] );
						v2.pos.Set( p_mesh->mp_dl->mp_pos_pool[(index2*3)+0],
									p_mesh->mp_dl->mp_pos_pool[(index2*3)+1],
									p_mesh->mp_dl->mp_pos_pool[(index2*3)+2] );
					}
					else
					{
						v0.pos.Set( p_scene->mp_pos_pool[(index0*3)+0],
									p_scene->mp_pos_pool[(index0*3)+1],
									p_scene->mp_pos_pool[(index0*3)+2] );
						v1.pos.Set( p_scene->mp_pos_pool[(index1*3)+0],
									p_scene->mp_pos_pool[(index1*3)+1],
									p_scene->mp_pos_pool[(index1*3)+2] );
						v2.pos.Set( p_scene->mp_pos_pool[(index2*3)+0],
									p_scene->mp_pos_pool[(index2*3)+1],
									p_scene->mp_pos_pool[(index2*3)+2] );
					}

					if ( p_scene->mp_col_pool )
					{
						if ( p_mesh->mp_dl->mp_col_pool )
						{
							v0.col = *((GXColor*)&p_mesh->mp_dl->mp_col_pool[cindex0]);
							v1.col = *((GXColor*)&p_mesh->mp_dl->mp_col_pool[cindex1]);
							v2.col = *((GXColor*)&p_mesh->mp_dl->mp_col_pool[cindex2]);
						}
						else
						{
							v0.col = *((GXColor*)&p_scene->mp_col_pool[cindex0]);
							v1.col = *((GXColor*)&p_scene->mp_col_pool[cindex1]);
							v2.col = *((GXColor*)&p_scene->mp_col_pool[cindex2]);
						}
					}

					if ( p_scene->mp_tex_pool )
					{
						v0.u = p_scene->mp_tex_pool[(tindex0*2)+0];
						v0.v = p_scene->mp_tex_pool[(tindex0*2)+1];
						v1.u = p_scene->mp_tex_pool[(tindex1*2)+0];
						v1.v = p_scene->mp_tex_pool[(tindex1*2)+1];
						v2.u = p_scene->mp_tex_pool[(tindex2*2)+0];
						v2.v = p_scene->mp_tex_pool[(tindex2*2)+1];
					}

					v0.pos[W] = v1.pos[W] = v2.pos[W] = 1.0f;		// Make sure they are points, not vectors
					
					// Push this tri onto the stack.
					triSubdivideStack.Push( &v0 );
					triSubdivideStack.Push( &v1 );
					triSubdivideStack.Push( &v2 );

					// Figure the area of this tri.
					Mth::Vector p( v1.pos[X] - v0.pos[X], v1.pos[Y] - v0.pos[Y], v1.pos[Z] - v0.pos[Z], 0.0f );
					Mth::Vector q( v2.pos[X] - v0.pos[X], v2.pos[Y] - v0.pos[Y], v2.pos[Z] - v0.pos[Z], 0.0f );
					Mth::Vector r(( p[Y] * q[Z] ) - ( q[Y] * p[Z] ), ( p[Z] * q[X] ) - ( q[Z] * p[X] ), ( p[X] * q[Y] ) - ( q[X] * p[Y] ), 0.0f);
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
			sNgcShatterInstanceDetails *p_details		= new sNgcShatterInstanceDetails( valid_tris, p_mesh, p_scene, format );
			sNgcVert				   *p_write_vertex	= p_details->mp_vertex_buffer;
			uint32						details_index	= 0;

			Mth::Vector					spread_center	= shatterVelocity * -shatterSpreadFactor;
			float						base_speed		= shatterVelocity.Length();

			spread_center[X] += p_mesh->mp_dl->m_sphere[0];
			spread_center[Y] += p_mesh->mp_dl->m_sphere[1];
			spread_center[Z] += p_mesh->mp_dl->m_sphere[2];
			
			// Add the tracking structure to the table.
			p_shatter_details_table->PutItem((uint32)p_details, p_details );
			
			// Process-subdivide the entire stack.
			sNgcVert *p_copy_vertex = p_write_vertex;
			while( subdivide_tri_stack( &p_write_vertex, p_mesh ));
					
			// Copy the (possibly subdivided) vertex data over.
			while( p_copy_vertex < p_write_vertex )
			{
				Mth::Vector *p_vert0 = &p_copy_vertex[0].pos;
				Mth::Vector *p_vert1 = &p_copy_vertex[1].pos;
				Mth::Vector *p_vert2 = &p_copy_vertex[2].pos;
				
				// Calculate position as the midpoint of the three vertices per poly.
				p_details->mp_positions[details_index][X] = ( p_vert0->GetX() + p_vert1->GetX() + p_vert2->GetX() ) * ( 1.0f / 3.0f );
				p_details->mp_positions[details_index][Y] = ( p_vert0->GetY() + p_vert1->GetY() + p_vert2->GetY() ) * ( 1.0f / 3.0f );
				p_details->mp_positions[details_index][Z] = ( p_vert0->GetZ() + p_vert1->GetZ() + p_vert2->GetZ() ) * ( 1.0f / 3.0f );

				// Calculate the vector <velocity> back from the bounding box of the object. Then use this to figure the 'spread' of the
				// shards by calculating the vector from this position to the center of each shard.
				float speed = base_speed + ( base_speed * (( shatterVelocityVariance * rand() ) / RAND_MAX ));
				p_details->mp_velocities[details_index] = ( p_details->mp_positions[details_index] - spread_center ).Normalize( speed );

				Mth::Vector axis( -1.0f + ( 2.0f * (float)rand() / RAND_MAX ), -1.0f + ( 2.0f * (float)rand() / RAND_MAX ), -1.0f + ( 2.0f * (float)rand() / RAND_MAX ));
				axis.Normalize();
				p_details->mp_matrices[details_index].Ident();
				p_details->mp_matrices[details_index].Rotate( axis, 0.1f * ((float)rand() / RAND_MAX ));

				p_copy_vertex += 3;
						
				++details_index;
			}
		}
	}
#endif
}



/******************************************************************************
 *
 * 
 *****************************************************************************/
void plat_shatter_update( sShatterInstanceDetails *p_details, float framelength )
{
	sNgcShatterInstanceDetails *p_Ngc_details = static_cast<sNgcShatterInstanceDetails*>( p_details );
	
	sNgcVert *p_vert_data = p_Ngc_details->mp_vertex_buffer;
	
	for( int i = 0; i < p_details->m_num_triangles; ++i )
	{
		Mth::Vector *p_v0	= &p_vert_data[(i*3)+0].pos;
		Mth::Vector *p_v1	= &p_vert_data[(i*3)+1].pos;
		Mth::Vector *p_v2	= &p_vert_data[(i*3)+2].pos;
		// To move the shatter pieces:
		// 1) subtract position from each vertex
		// 2) rotate
		// 3) update position with velocity
		// 4) add new position to each vertex

		// The matrix holds 3 vectors at once.
		Mth::Matrix m;
		m[X].Set( p_v0->GetX() - p_details->mp_positions[i][X], p_v0->GetY() - p_details->mp_positions[i][Y], p_v0->GetZ() - p_details->mp_positions[i][Z] );
		m[Y].Set( p_v1->GetX() - p_details->mp_positions[i][X], p_v1->GetY() - p_details->mp_positions[i][Y], p_v1->GetZ() - p_details->mp_positions[i][Z] );
		m[Z].Set( p_v2->GetX() - p_details->mp_positions[i][X], p_v2->GetY() - p_details->mp_positions[i][Y], p_v2->GetZ() - p_details->mp_positions[i][Z] );
         
		m[X].Rotate( p_details->mp_matrices[i] );
		m[Y].Rotate( p_details->mp_matrices[i] );
		m[Z].Rotate( p_details->mp_matrices[i] );

		// Update the position and velocity of the shatter piece, dealing with bouncing if necessary.
		p_details->UpdateParameters( i, framelength );
      
		m[X] += p_details->mp_positions[i]; 
		m[Y] += p_details->mp_positions[i]; 
		m[Z] += p_details->mp_positions[i];

		p_v0->Set( m[X][X], m[X][Y], m[X][Z] );
		p_v1->Set( m[Y][X], m[Y][Y], m[Y][Z] );
		p_v2->Set( m[Z][X], m[Z][Y], m[Z][Z] );
	}
}



/******************************************************************************
 *
 * 
 *****************************************************************************/
void plat_shatter_render( sShatterInstanceDetails *p_details )
{
	sNgcShatterInstanceDetails *p_Ngc_details = static_cast<sNgcShatterInstanceDetails*>( p_details );

	Dbg_Assert( p_Ngc_details );

//	p_Ngc_details->mp_mesh->mp_material->Submit( 0, (GXColor){0,0,0,0} );

	NxNgc::MaterialSubmit( p_Ngc_details->mp_mesh, p_Ngc_details->mp_scene );
	GX::SetChanCtrl( GX_COLOR0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
	GX::SetChanCtrl( GX_ALPHA0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 
//	GX::SetChanMatColor( GX_COLOR0A0, p_Ngc_details->mp_mesh->m_base_color );
	GX::SetChanMatColor( GX_COLOR0A0, (GXColor){128,128,128,255} );

//	GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_IDENTITY );

	// Render the triangles
	GX::SetVtxDesc( 3, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );
	GX::Begin( GX_TRIANGLES, p_Ngc_details->m_vertex_format, p_Ngc_details->m_num_triangles * 3 ); 
	sNgcVert *p_vert_array = p_Ngc_details->mp_vertex_buffer;
	for (int i = 0; i < p_Ngc_details->m_num_triangles; i++, p_vert_array += 3)
	{
		GX::Position3f32( p_vert_array[0].pos[X], p_vert_array[0].pos[Y], p_vert_array[0].pos[Z] );
		GX::Color1u32(*((u32*)&(p_vert_array[0].col)));
		GX::TexCoord2s16( p_vert_array[0].u, p_vert_array[0].v );

		GX::Position3f32( p_vert_array[1].pos[X], p_vert_array[1].pos[Y], p_vert_array[1].pos[Z] );
		GX::Color1u32(*((u32*)&(p_vert_array[1].col)));
		GX::TexCoord2s16( p_vert_array[1].u, p_vert_array[1].v );

		GX::Position3f32( p_vert_array[2].pos[X], p_vert_array[2].pos[Y], p_vert_array[2].pos[Z] );
		GX::Color1u32(*((u32*)&(p_vert_array[2].col)));
		GX::TexCoord2s16( p_vert_array[2].u, p_vert_array[2].v );
	}
	GX::End();
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

static bool		fogEnabled			= false;
static GXColor	fogColor			= (GXColor){ 0, 0, 0, 0 };
static float	fogNear				= 0.0f;
static float	fogFar				= 0.0f;

void		CFog::s_plat_enable_fog(bool enable)
{
	if( enable != fogEnabled )
	{
		// If we're disabling, reset to default values.
		if ( !enable )
		{
			fogColor	= (GXColor){ 0, 0, 0, 0 };
			fogNear		= 0.0f;
			fogFar		= 0.0f;
		}

		fogEnabled = enable;
	}
}

void		CFog::s_plat_set_fog_exponent(float exponent)
{
//	Dbg_Message("Stub: CFog::SetFogExponent()");
}

void		CFog::s_plat_set_fog_rgba(Image::RGBA rgba)
{
	fogColor.r = rgba.r;
	fogColor.g = rgba.g;
	fogColor.b = rgba.b;
	fogColor.a = rgba.a;
//	fogFar = 1000.0f * ( 128.0f / (float)fogColor.a );
	fogFar = 20000.0f * ( 128.0f / (float)fogColor.a ); 
}

void		CFog::s_plat_set_fog_near_distance(float distance)
{
	fogNear = distance;
//	fogFar = 1000.0f * ( 128.0f / (float)fogColor.a );
	fogFar = 20000.0f * ( 128.0f / (float)fogColor.a ); 
}

void		CFog::s_plat_set_fog_color( void )
{
	GX::SetFogColor( fogColor );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CFog::s_plat_fog_update( void )
{
	if( fogEnabled )
	{
		GX::SetFog( GX_FOG_EXP, fogNear, fogFar, 2.0f, 20000.0f, fogColor );
	}
	else
	{
		GX::SetFog( GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, fogColor );
	}
}
	


} // Nx



