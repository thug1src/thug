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

#include <gfx/ngps/p_nxgeom.h>
#include <gfx/ngps/p_nxtexture.h>

#include <gfx/ngps/nx/dma.h>
#include <gfx/ngps/nx/fx.h>
#include <gfx/ngps/nx/immediate.h>
#include <gfx/ngps/nx/geomnode.h>
#include <gfx/ngps/nx/group.h>
#include <gfx/ngps/nx/gs.h>
#include <gfx/ngps/nx/vif.h>
#include <gfx/ngps/nx/render.h>
#include <gfx/ngps/nx/switches.h>

namespace Nx
{

#define	DRAW_DEBUG_LINES		0
#define PRINT_SHATTER_MEMORY	0


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sNGPSVert
{
	Mth::Vector		pos;
	Image::RGBA		col;
	float			u, v;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sNGPSSplatInstanceDetails : public sSplatInstanceDetails
{
	// Platform specific part.
	NxPs2::SSingleTexture	*mp_texture;
	sNGPSVert				m_verts[SPLAT_POLYS_PER_MESH * 3];
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sNGPSShatterInstanceDetails : public sShatterInstanceDetails
{
	// Platform specific part.
							sNGPSShatterInstanceDetails( int num_tris, NxPs2::CGeomNode *p_geom_node );
							~sNGPSShatterInstanceDetails( void );

	static int				sQueryMemoryNeeded(int num_tris);

	NxPs2::CGeomNode *		mp_geom_node;		// Corresponding source mesh
	sNGPSVert *				mp_vert_array;		// Array of vertices

	uint32 *				mp_tex_regs;		// packed texture registers
	int						m_num_tex_regs;		// number of texture registers
};



/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sNGPSShatterInstanceDetails::sNGPSShatterInstanceDetails( int num_tris, NxPs2::CGeomNode *p_geom_node ) : sShatterInstanceDetails( num_tris )
{
	mp_geom_node = p_geom_node;

#if PRINT_SHATTER_MEMORY
	Dbg_Message("Allocating %d bytes for vert array of %d tris", sizeof(sNGPSVert) * 3 * num_tris, num_tris);
#endif
	mp_vert_array = new sNGPSVert[num_tris * 3];

	// Get texture info
	if (p_geom_node->GetGroup())		// Garrett: Need to find the real way to figure out if a mesh
	{									// is textures or not (can always look at the GIFtag)
		uint32 *p_dma = (uint32 *) p_geom_node->GetDma();

		p_dma += 9;			// Go to vif::UNPACK() for texture regs

		m_num_tex_regs = (*p_dma >> 16) & 0xf;
		mp_tex_regs = p_dma + 1;
#if 0
		Dbg_Message("Found %d texture registers for type %x", m_num_tex_regs, (*p_dma >> 24));
		for (int i = 0; i < m_num_tex_regs; i++)
		{
			Dbg_Message(" - Register %x", mp_tex_regs[(i * 3) + 2]);
		}
#endif
	} else {
		mp_tex_regs = NULL;
		m_num_tex_regs = 0;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sNGPSShatterInstanceDetails::~sNGPSShatterInstanceDetails( void )
{
	delete [] mp_vert_array;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			sNGPSShatterInstanceDetails::sQueryMemoryNeeded(int num_tris)
{
	int size = sShatterInstanceDetails::s_query_memory_needed(num_tris);

	size += (sizeof(sNGPSShatterInstanceDetails) - sizeof(sShatterInstanceDetails));
	size += sizeof(sNGPSVert) * num_tris * 3;

	return size;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sNGPSSplatInstanceDetails * getDetailsForTextureSplat( NxPs2::SSingleTexture *p_texture )
{
	sNGPSSplatInstanceDetails *p_ngps_details;

	Dbg_Assert( p_splat_details_table );
	
	// Check to see whether we have a scene already created for this type of texture splat.
	p_splat_details_table->IterateStart();
	sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();
	while( p_details )
	{
		p_ngps_details					= static_cast<sNGPSSplatInstanceDetails*>( p_details );

		// If this one matches, return it...
		if (p_ngps_details->mp_texture == p_texture)
		{
			return p_ngps_details;
		}
		p_details = p_splat_details_table->IterateNext();
	}

	// Check to see that we have memory first
	if (Mem::Available() < (int) ((sizeof(sNGPSSplatInstanceDetails) * 3) / 2))
	{
		return NULL;
	}

	p_ngps_details = new sNGPSSplatInstanceDetails;
	p_ngps_details->m_highest_active_splat	= 0;
	p_ngps_details->mp_texture = p_texture;
	memset( p_ngps_details->m_lifetimes, 0, sizeof( int ) * SPLAT_POLYS_PER_MESH );
	
	for( int v = 0; v < SPLAT_POLYS_PER_MESH * 3; ++v )
	{
		p_ngps_details->m_verts[v].pos = Mth::Vector(0.0f, 0.0f, 0.0f, 1.0f);
		p_ngps_details->m_verts[v].col = Image::RGBA( 0x80, 0x80, 0x80, 0x80 );
	}

	p_splat_details_table->PutItem((uint32)p_ngps_details, p_ngps_details );

	return p_ngps_details;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool subdivide_tri_stack( float shatterArea, sNGPSVert **p_write, NxPs2::CGeomNode *p_node )
{
	// If there are elements on the stack, pop off the top three vertices and subdivide if necessary.
	if( triSubdivideStack.IsEmpty())
	{
		return false;
	}

	// Three temporary buffers.
	sNGPSVert v0;
	sNGPSVert v1;
	sNGPSVert v2;
	
	// Stack is LIFO, so Pop() off in reverse order.
	triSubdivideStack.Pop( &v2 );
	triSubdivideStack.Pop( &v1 );
	triSubdivideStack.Pop( &v0 );
	
	// Calculate the area of this tri.
	Mth::Vector p(	v1.pos[X] - v0.pos[X], v1.pos[Y] - v0.pos[Y], v1.pos[Z] - v0.pos[Z] );
	Mth::Vector q(	v2.pos[X] - v0.pos[X], v2.pos[Y] - v0.pos[Y], v2.pos[Z] - v0.pos[Z] );
	Mth::Vector r(( p[Y] * q[Z] ) - ( q[Y] * p[Z] ), ( p[Z] * q[X] ) - ( q[Z] * p[X] ), ( p[X] * q[Y] ) - ( q[X] * p[Y] ));
	float area_squared = r.LengthSqr();

	if( area_squared > shatterArea )
	{
		// We need to subdivide this tri. Calculate the three intermediate points.
		int block_size = triSubdivideStack.GetBlockSize();

		// Three temporary buffers.
		sNGPSVert i01;
		sNGPSVert i12;
		sNGPSVert i20;

		memcpy( &i01, &v0, block_size );
		memcpy( &i12, &v1, block_size );
		memcpy( &i20, &v2, block_size );

		// Deal with positions (always present).
		i01.pos[X] = v0.pos[X] + (( v1.pos[X] - v0.pos[X] ) * 0.5f );
		i01.pos[Y] = v0.pos[Y] + (( v1.pos[Y] - v0.pos[Y] ) * 0.5f );
		i01.pos[Z] = v0.pos[Z] + (( v1.pos[Z] - v0.pos[Z] ) * 0.5f );

		i12.pos[X] = v1.pos[X] + (( v2.pos[X] - v1.pos[X] ) * 0.5f );
		i12.pos[Y] = v1.pos[Y] + (( v2.pos[Y] - v1.pos[Y] ) * 0.5f );
		i12.pos[Z] = v1.pos[Z] + (( v2.pos[Z] - v1.pos[Z] ) * 0.5f );

		i20.pos[X] = v2.pos[X] + (( v0.pos[X] - v2.pos[X] ) * 0.5f );
		i20.pos[Y] = v2.pos[Y] + (( v0.pos[Y] - v2.pos[Y] ) * 0.5f );
		i20.pos[Z] = v2.pos[Z] + (( v0.pos[Z] - v2.pos[Z] ) * 0.5f );

		// Deal with colors (not always present).
		//if( p_node->m_diffuse_offset > 0 )
		{
			uint8	*p_v0col	= (uint8*)( &(v0.col));
			uint8	*p_v1col	= (uint8*)( &(v1.col));
			uint8	*p_v2col	= (uint8*)( &(v2.col));
			uint8	*p_i01col	= (uint8*)( &(i01.col));
			uint8	*p_i12col	= (uint8*)( &(i12.col));
			uint8	*p_i20col	= (uint8*)( &(i20.col));
		
			for( int i = 0; i < 4; ++i )
			{
				p_i01col[i]		= p_v0col[i] + (char)(( p_v1col[i] - p_v0col[i] ) * 0.5f );
				p_i12col[i]		= p_v1col[i] + (char)(( p_v2col[i] - p_v1col[i] ) * 0.5f );
				p_i20col[i]		= p_v2col[i] + (char)(( p_v0col[i] - p_v2col[i] ) * 0.5f );
			}
		}

		// Deal with uv0 (not always present).
		//if( p_node->m_uv0_offset > 0 )
		{
			float	*p_v0uv		= (float*)( &v0.u );
			float	*p_v1uv		= (float*)( &v1.u );
			float	*p_v2uv		= (float*)( &v2.u );
			float	*p_i01uv	= (float*)( &i01.u );
			float	*p_i12uv	= (float*)( &i12.u );
			float	*p_i20uv	= (float*)( &i20.u );
		
			// We know that v is contiguous to u
			for( int i = 0; i < 2; ++i )
			{
				p_i01uv[i]		= p_v0uv[i] + (( p_v1uv[i] - p_v0uv[i] ) * 0.5f );
				p_i12uv[i]		= p_v1uv[i] + (( p_v2uv[i] - p_v1uv[i] ) * 0.5f );
				p_i20uv[i]		= p_v2uv[i] + (( p_v0uv[i] - p_v2uv[i] ) * 0.5f );
			}
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
		// Don't need to subdivide this tri.
		int block_size = triSubdivideStack.GetBlockSize();

		// Just copy the tri into the next available slot.
		memcpy( *p_write, &v0, block_size );
		(*p_write)++;
		memcpy( *p_write, &v1, block_size );
		(*p_write)++;
		memcpy( *p_write, &v2, block_size );
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
	// Get viewport details.
	CViewport *p_vp = CViewportManager::sGetActiveViewport( p_details->m_viewport );
	
	// Centre of screen is (0x8000, 0x8000), unit is 1/16 pixel.
	
	int x0 = XOFFSET + (int)( p_vp->GetOriginX() * (HRES << 4) );
	int y0 = YOFFSET + (int)( p_vp->GetOriginY() * (VRES << 4) );
	int x1 = x0 + (int)( p_vp->GetWidth() * (HRES << 4) );
	int y1 = y0 + (int)( p_vp->GetHeight() * (VRES << 4) );

	uint32 rgba = p_details->m_current.r | ((uint32)p_details->m_current.g << 8 ) | ((uint32)p_details->m_current.b << 16 ) | ((uint32)p_details->m_current.a << 24 );
	
	NxPs2::DrawRectangle( x0, y0, x1, y1, rgba );
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
	sNGPSSplatInstanceDetails *p_ngps_details;

	Dbg_Assert( p_splat_details_table );
	
	p_splat_details_table->IterateStart();
	sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();
	while( p_details )
	{
		p_ngps_details = static_cast<sNGPSSplatInstanceDetails*>( p_details );
		
		p_details = p_splat_details_table->IterateNext();

		p_splat_details_table->FlushItem((uint32)p_ngps_details );
		delete p_ngps_details;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_texture_splat_reset_poly( sSplatInstanceDetails *p_details, int index )
{
	// Cast the details to NGPS details.
	sNGPSSplatInstanceDetails *p_ngps_details = static_cast<sNGPSSplatInstanceDetails *>( p_details );
	
	// Force this poly to be degenerate.
	p_ngps_details->m_verts[index * 3 + 1]	= p_ngps_details->m_verts[index * 3];
	p_ngps_details->m_verts[index * 3 + 2]	= p_ngps_details->m_verts[index * 3];
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool plat_texture_splat( Nx::CSector **pp_sectors, Nx::CCollStatic **pp_collision, Mth::Vector& start, Mth::Vector& end,
						 float size, float lifetime, Nx::CTexture *p_texture, Nx::sSplatTrailInstanceDetails *p_trail_details )
{
	Mth::Matrix view_matrix, ortho_matrix, projection_matrix;

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
	if( p_trail_details )
	{
		Mth::Vector up( start[X] - p_trail_details->m_last_pos[X], start[Y] - p_trail_details->m_last_pos[Y], start[Z] - p_trail_details->m_last_pos[Z], 0.0f );
		Dbg_MsgAssert((up[X] != 0.0f) || (up[Z] != 0.0f), ("Up vector has no length"));
		//Dbg_Message("Splat up vector (%.8f, %.8f, %.8f)", up[X], up[Y], up[Z]);

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

	// Find texture info
	Dbg_Assert(p_texture);
	CPs2Texture *p_ps2_texture = static_cast<CPs2Texture *>(p_texture);
	NxPs2::SSingleTexture *p_single_texture = p_ps2_texture->GetSingleTexture();
	Dbg_Assert(p_single_texture);

	// Pointer to the mesh we will be modifying. (Don't want to set the pointer up until we know for
	// sure that we will be adding some polys).
	sNGPSSplatInstanceDetails	*p_details		= NULL;
	sNGPSVert					*p_target_verts	= NULL;

#if DRAW_DEBUG_LINES
	Nx::CSector *p_sector;
	while(( p_sector = *pp_sectors ))
	{
		Nx::CPs2Geom *p_ngps_geom = static_cast<Nx::CPs2Geom*>( p_sector->GetGeom());

		if( p_ngps_geom )
		{
			Mth::Vector min = p_ngps_geom->GetBoundingBox().GetMin();
			Mth::Vector max = p_ngps_geom->GetBoundingBox().GetMax();

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
		}

		++pp_sectors;
	}
#endif // DRAW_DEBUG_LINES

	// Make initial line bounding box.  (Possibly pass previous one in if this takes too long)
	Mth::CBBox line_bbox( start );
	line_bbox.AddPoint( end );

	//Dbg_Message("Using line (%f, %f, %f) - (%f, %f, %f)", start[X], start[Y], start[Z], end[X], end[Y], end[Z]);

	Nx::CCollStatic *p_collision;
	while(( p_collision = *(pp_collision++) ))
	{
		// Since the collision is static, we can assume the collision geometry data is in world space
		Nx::CCollObjTriData *p_coll_geom = p_collision->GetGeometry();
		
		// narrow down list of possible faces
		uint num_faces;
		FaceIndex *p_face_indexes;
		p_face_indexes = p_coll_geom->FindIntersectingFaces(line_bbox, num_faces);
		Dbg_Assert(p_face_indexes);

		Mth::Vector uvprojections[3];

		for (uint fidx = 0; fidx < num_faces; fidx++, p_face_indexes++)
		{
			// Make sure it is collidable and visible
			if (p_coll_geom->GetFaceFlags(*p_face_indexes) & (mFD_NON_COLLIDABLE | mFD_INVISIBLE))
			{
				continue;
			}

			Mth::Vector v0(p_coll_geom->GetRawVertexPos(p_coll_geom->GetFaceVertIndex(*p_face_indexes, 0)));
			Mth::Vector v1(p_coll_geom->GetRawVertexPos(p_coll_geom->GetFaceVertIndex(*p_face_indexes, 1)));
			Mth::Vector v2(p_coll_geom->GetRawVertexPos(p_coll_geom->GetFaceVertIndex(*p_face_indexes, 2)));
			v0[W] = v1[W] = v2[W] = 1.0f;		// Make sure they are points, not vectors

#if 1
			uvprojections[0] = v0 * projection_matrix;
			uvprojections[1] = v1 * projection_matrix;
			uvprojections[2] = v2 * projection_matrix;
#else
			// Test each component
			uvprojections[0] = v0 * view_matrix;
			uvprojections[1] = v1 * view_matrix;
			uvprojections[2] = v2 * view_matrix;

			Dbg_Message("Looking at possible splat poly vert 0 (%f, %f, %f, %f)", v0[X], v0[Y], v0[Z], v0[W]);
			Dbg_Message("Looking at possible splat poly vert 1 (%f, %f, %f, %f)", v1[X], v1[Y], v1[Z], v1[W]);
			Dbg_Message("Looking at possible splat poly vert 2 (%f, %f, %f, %f)", v2[X], v2[Y], v2[Z], v2[W]);

			Dbg_Message("Converted vert 0 to (%f, %f, %f)", uvprojections[0][X], uvprojections[0][Y], uvprojections[0][Z]);
			Dbg_Message("Converted vert 1 to (%f, %f, %f)", uvprojections[1][X], uvprojections[1][Y], uvprojections[1][Z]);
			Dbg_Message("Converted vert 2 to (%f, %f, %f)", uvprojections[2][X], uvprojections[2][Y], uvprojections[2][Z]);

			uvprojections[0] = uvprojections[0] * ortho_matrix;
			uvprojections[1] = uvprojections[1] * ortho_matrix;
			uvprojections[2] = uvprojections[2] * ortho_matrix;

			Dbg_Message("Converted vert 0 again to (%f, %f, %f)", uvprojections[0][X], uvprojections[0][Y], uvprojections[0][Z]);
			Dbg_Message("Converted vert 1 again to (%f, %f, %f)", uvprojections[1][X], uvprojections[1][Y], uvprojections[1][Z]);
			Dbg_Message("Converted vert 2 again to (%f, %f, %f)", uvprojections[2][X], uvprojections[2][Y], uvprojections[2][Z]);
#endif

			// Check that they are in the frustum
//			if(( uvprojections[0][X] < -1.0f ) && ( uvprojections[1][X] < -1.0f ) && ( uvprojections[2][X] < -1.0f ))
//				continue;
//			if(( uvprojections[0][Y] < -1.0f ) && ( uvprojections[1][Y] < -1.0f ) && ( uvprojections[2][Y] < -1.0f ))
//				continue;
//			if(( uvprojections[0][X] > 1.0f ) && ( uvprojections[1][X] > 1.0f ) && ( uvprojections[2][X] > 1.0f ))
//				continue;
//			if(( uvprojections[0][Y] > 1.0f ) && ( uvprojections[1][Y] > 1.0f ) && ( uvprojections[2][Y] > 1.0f ))
//				continue;
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

			//Dbg_Message("Found splat poly vert 0 (%f, %f, %f) uv (%f, %f)", v0[X], v0[Y], v0[Z], uvprojections[0][X], uvprojections[0][Y]);
			//Dbg_Message("Found splat poly vert 1 (%f, %f, %f) uv (%f, %f)", v1[X], v1[Y], v1[Z], uvprojections[1][X], uvprojections[1][Y]);
			//Dbg_Message("Found splat poly vert 2 (%f, %f, %f) uv (%f, %f)", v2[X], v2[Y], v2[Z], uvprojections[2][X], uvprojections[2][Y]);

			// Check if we have a line or point.  For some reason, the rendering code will crash
			// with this (I think it is the clipping).  Since we really don't want it anyways,
			// just skip them.
			int equal_axis = 0;
			if ((v0[X] == v1[X]) && (v1[X] == v2[X]))	equal_axis++;
			if ((v0[Y] == v1[Y]) && (v1[Y] == v2[Y]))	equal_axis++;
			if ((v0[Z] == v1[Z]) && (v1[Z] == v2[Z]))	equal_axis++;
			if (equal_axis > 1)
			{
				//Dbg_Message("Skipping line poly...");
				continue;
			}

			// Okay, this tri lies within the projection frustum. Get a pointer to the mesh used for rendering texture splats
			// with the given texture. (Note this will create a new instance to handle texture splats of this texture if one
			// does not already exist).
			if( p_target_verts == NULL )
			{
				p_details						= getDetailsForTextureSplat(p_single_texture);
				if ( p_details == NULL )
				{
					continue;
				}

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
			Dbg_Assert((index + 2) < (SPLAT_POLYS_PER_MESH * 3));
			p_target_verts[index + 0].pos	= v0;
			p_target_verts[index + 1].pos	= v1;
			p_target_verts[index + 2].pos	= v2;
			//p_target_verts[index + 0].pos[Y] += ( 12.0f * (float)rand() / RAND_MAX );
			//p_target_verts[index + 1].pos[Y] += ( 12.0f * (float)rand() / RAND_MAX );
			//p_target_verts[index + 2].pos[Y] += ( 12.0f * (float)rand() / RAND_MAX );

			// Then the uv's.
			p_target_verts[index + 0].u		= ( (uvprojections[0][X] * 0.5f) + 0.5f ) * up_one_u_pixel;
			p_target_verts[index + 0].v		= ( (uvprojections[0][Y] * 0.5f) + 0.5f ) * up_one_v_pixel;
			p_target_verts[index + 1].u		= ( (uvprojections[1][X] * 0.5f) + 0.5f ) * up_one_u_pixel;
			p_target_verts[index + 1].v		= ( (uvprojections[1][Y] * 0.5f) + 0.5f ) * up_one_v_pixel;
			p_target_verts[index + 2].u		= ( (uvprojections[2][X] * 0.5f) + 0.5f ) * up_one_u_pixel;
			p_target_verts[index + 2].v		= ( (uvprojections[2][Y] * 0.5f) + 0.5f ) * up_one_v_pixel;

			// Now the colors
			uint8 intensity0 = p_coll_geom->GetVertexIntensity(p_coll_geom->GetFaceVertIndex(*p_face_indexes, 0));
			uint8 intensity1 = p_coll_geom->GetVertexIntensity(p_coll_geom->GetFaceVertIndex(*p_face_indexes, 1));
			uint8 intensity2 = p_coll_geom->GetVertexIntensity(p_coll_geom->GetFaceVertIndex(*p_face_indexes, 2));
			p_target_verts[index + 0].col = Image::RGBA(intensity0, intensity0, intensity0, 0x80);
			p_target_verts[index + 1].col = Image::RGBA(intensity1, intensity1, intensity1, 0x80);
			p_target_verts[index + 2].col = Image::RGBA(intensity2, intensity2, intensity2, 0x80);

			Mth::Vector	*p_v0 = &( p_target_verts[index + 0].pos );
			Mth::Vector	*p_v1 = &( p_target_verts[index + 1].pos );
			Mth::Vector	*p_v2 = &( p_target_verts[index + 2].pos );
			Mth::Vector pv(	p_v1->GetX() - p_v0->GetX(), p_v1->GetY() - p_v0->GetY(), p_v1->GetZ() - p_v0->GetZ() );
			Mth::Vector qv(	p_v2->GetX() - p_v0->GetX(), p_v2->GetY() - p_v0->GetY(), p_v2->GetZ() - p_v0->GetZ() );
			Mth::Vector r(( pv[Y] * qv[Z] ) - ( qv[Y] * pv[Z] ), ( pv[Z] * qv[X] ) - ( qv[Z] * pv[X] ), ( pv[X] * qv[Y] ) - ( qv[X] * pv[Y] ));
			float area_squared = r.LengthSqr();

			// Set the shatter test to ensure that we don't subdivide too far. Note that each successive subdivision will quarter
			// the area of each triangle, which means the area *squared* of each triangle will become 1/16th of the previous value.
			float shatterArea = area_squared / 128.0f;

			triSubdivideStack.Clear();
			triSubdivideStack.SetBlockSize( sizeof(sNGPSVert) );
			triSubdivideStack.Push( &p_target_verts[index + 0] );
			triSubdivideStack.Push( &p_target_verts[index + 1] );
			triSubdivideStack.Push( &p_target_verts[index + 2] );

			// Allocate a block of memory into which the subdivision stack will write the results.
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
			sNGPSVert		*p_array		= new sNGPSVert[100];
			sNGPSVert		*p_array_start	= p_array;
			sNGPSVert		*p_array_loop	= p_array;
			//memset( p_array, 0, sizeof(sNGPSVert) * 100 );
			Mem::Manager::sHandle().PopContext();

			while( subdivide_tri_stack( shatterArea, &p_array, NULL ));

			// Ensure we haven't overrun the buffer.
			Dbg_Assert((uint32)p_array - (uint32)p_array_loop < ( sizeof(sNGPSVert) * 100 ));

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
		}
	}
	
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void plat_texture_splat_render( void )
{
	sNGPSSplatInstanceDetails *p_ngps_details;

	Dbg_Assert( p_splat_details_table );

	p_splat_details_table->IterateStart();
	sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();

	NxPs2::dma::BeginTag(NxPs2::dma::cnt, 0);

	bool set_zpush = (p_details != NULL);
	if (set_zpush)
	{
		NxPs2::CImmediateMode::sSetZPush(24.0f);
	}

	while( p_details )
	{

		// See if we have anything to draw first
		if (p_details->m_highest_active_splat >= 0)
		{
			p_ngps_details = static_cast<sNGPSSplatInstanceDetails*>( p_details );

			// Init drawing, since we know we have to draw
#if 1
			NxPs2::CImmediateMode::sStartPolyDraw(p_ngps_details->mp_texture, PackALPHA(0,1,0,1,0), ABS, true);
#else
			if ((Tmr::GetVblanks() % 600) < 300)
			{
				NxPs2::CImmediateMode::sStartPolyDraw(p_ngps_details->mp_texture, PackALPHA(0,0,0,0,0), ABS, true);
			} else {
				NxPs2::CImmediateMode::sStartPolyDraw(p_ngps_details->mp_texture, PackALPHA(0,0,0,0,0), ABS, false);
			}
#endif

			// Render the triangles
			sNGPSVert *p_vert_array = p_ngps_details->m_verts;
			for (int i = 0; i <= p_details->m_highest_active_splat; i++, p_vert_array += 3)
			{
				NxPs2::CImmediateMode::sDrawTriUV(p_vert_array[0].pos, p_vert_array[1].pos, p_vert_array[2].pos,
												  p_vert_array[0].u, p_vert_array[0].v,
												  p_vert_array[1].u, p_vert_array[1].v,
												  p_vert_array[2].u, p_vert_array[2].v,
												  *((uint32 *) &p_vert_array[0].col),
												  *((uint32 *) &p_vert_array[1].col),
												  *((uint32 *) &p_vert_array[2].col),
												  ABS);

#if DRAW_DEBUG_LINES
				Gfx::AddDebugLine( p_vert_array[0].pos, p_vert_array[1].pos, MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
				Gfx::AddDebugLine( p_vert_array[1].pos, p_vert_array[2].pos, MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
				Gfx::AddDebugLine( p_vert_array[2].pos, p_vert_array[0].pos, MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
#endif
			}
		}
		
		p_details = p_splat_details_table->IterateNext();
	}

	if (set_zpush)
	{
		NxPs2::CImmediateMode::sClearZPush();
	}

	NxPs2::dma::EndTag();
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
	sNGPSShatterInstanceDetails *p_ngps_details;

	Dbg_Assert( p_shatter_details_table );
	
	p_shatter_details_table->IterateStart();
	sShatterInstanceDetails *p_details = p_shatter_details_table->IterateNext();
	while( p_details )
	{
		p_ngps_details = static_cast<sNGPSShatterInstanceDetails*>( p_details );
		
		p_details = p_shatter_details_table->IterateNext();

		p_shatter_details_table->FlushItem((uint32)p_ngps_details );
		delete p_ngps_details;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const int MAX_GEOM_NODES = 10;

void find_geom_leaves( NxPs2::CGeomNode *p_node, NxPs2::CGeomNode **leaf_array, int & num_nodes)
{
	if (p_node->IsLeaf())
	{
		Dbg_Assert(num_nodes < MAX_GEOM_NODES);

		leaf_array[num_nodes++] = p_node;
		return;
	} else {
		NxPs2::CGeomNode *p_child;
		for (p_child = p_node->GetChild(); p_child; p_child = p_child->GetSibling())
		{
			find_geom_leaves(p_child, leaf_array, num_nodes);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int get_num_extra_shatter_tris(float shatterArea, const Mth::Vector & pos0, const Mth::Vector & pos1, const Mth::Vector & pos2)
{
	int num_extra_tris = 0;

	// Figure the area of this tri.
	Mth::Vector p( pos1[X] - pos0[X], pos1[Y] - pos0[Y], pos1[Z] - pos0[Z], 0.0f );
	Mth::Vector q( pos2[X] - pos0[X], pos2[Y] - pos0[Y], pos2[Z] - pos0[Z], 0.0f );
	Mth::Vector r(( p[Y] * q[Z] ) - ( q[Y] * p[Z] ), ( p[Z] * q[X] ) - ( q[Z] * p[X] ), ( p[X] * q[Y] ) - ( q[X] * p[Y] ), 0.0f);
	float area_squared = r.LengthSqr();

	if( area_squared > shatterArea )
	{
		//Dbg_Message("Area of triangle: %f", sqrtf(area_squared));

		// We will need to subdivide - each subdivision will result in an area one quarter the previous area
		// (and thusly the square of the area will be one sixteenth the previous area).
		num_extra_tris = 1;
		while( area_squared > shatterArea )
		{
			num_extra_tris *= 4;
			area_squared *= ( 1.0f / 16.0f );
		}

		// This original tri will not be added...
		// ...however, the subdivided versions will.
		num_extra_tris--;
		//Dbg_Message("Adding %d triangles", num_extra_tris);
	}

	return num_extra_tris;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void plat_shatter( CGeom *p_geom )
{
	CPs2Geom *p_ps2_geom = static_cast<CPs2Geom*>( p_geom );
	NxPs2::CGeomNode *p_root_node = p_ps2_geom->GetEngineObject();
	Dbg_Assert(p_root_node);

	// Get Time of Day color
	Image::RGBA geom_rgba = Nx::CEngine::sGetMainScene()->GetMajorityColor();

	// Put all the meshes in an array
	int num_meshes = 0;
	NxPs2::CGeomNode *mesh_array[MAX_GEOM_NODES];
	find_geom_leaves(p_root_node, mesh_array, num_meshes);

	// For each mesh in the geom...
	for( int m = 0; m < num_meshes; ++m )
	{
		NxPs2::CGeomNode *p_node = mesh_array[m];

		// Since these are extra passes 99% of the time, Environment Mapped CGeomNodes are rejected
		if (p_node->IsEnvMapped())
		{
			continue;
		}

		uint8 *p_dma = p_node->GetDma();

		int num_verts = NxPs2::dma::GetNumVertices(p_dma);
		if( num_verts >= 3 )
		{
			bool short_xyz = (NxPs2::dma::GetBitLengthXYZ(p_dma) == 16);
			Mth::Vector mesh_center = p_node->GetBoundingSphere();

			// Set the block size for this mesh.
			triSubdivideStack.SetBlockSize( sizeof(sNGPSVert) );

			// Get DMA arrays
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
			sint32 *p_vert_array = new sint32[4 * num_verts];
			sint32 *p_uv_array = new sint32[2 * num_verts];
			uint32 *p_rgb_array = new uint32[num_verts];
#if PRINT_SHATTER_MEMORY
			Dbg_Message("Mesh #%d; Verts %d; CGeomNode pointer %x checksum %x", m, num_verts, p_node, p_node->GetChecksum());
			Dbg_Message("Allocating %d bytes temporarily for DMA", 7 * num_verts * sizeof(uint32));
#endif
			Mem::Manager::sHandle().PopContext();

			// Copy DMA data
			NxPs2::dma::ExtractXYZs(p_dma, (uint8 *) p_vert_array);
			NxPs2::dma::ExtractSTs(p_dma, (uint8 *) p_uv_array);
			NxPs2::dma::ExtractRGBAs(p_dma, (uint8 *) p_rgb_array);

			Dbg_Assert(p_vert_array[(0 * 4) + 3] & 0x8000);
			Dbg_Assert(p_vert_array[(1 * 4) + 3] & 0x8000);

			sNGPSVert v0, v1, v2;

			if (short_xyz)
			{
				NxPs2::dma::ConvertXYZToFloat(v0.pos, &(p_vert_array[0 * 4]), mesh_center);
				NxPs2::dma::ConvertXYZToFloat(v1.pos, &(p_vert_array[1 * 4]), mesh_center);
			} else {
				NxPs2::dma::ConvertXYZToFloat(v0.pos, &(p_vert_array[0 * 4]));
				NxPs2::dma::ConvertXYZToFloat(v1.pos, &(p_vert_array[1 * 4]));
			}
			NxPs2::dma::ConvertSTToFloat(v0.u, v0.v, &(p_uv_array[0 * 2]));
			NxPs2::dma::ConvertSTToFloat(v1.u, v1.v, &(p_uv_array[1 * 2]));

			v0.col = *((Image::RGBA *) &(p_rgb_array[0]));
			v0.col.Blend128(geom_rgba);
			v1.col = *((Image::RGBA *) &(p_rgb_array[1]));
			v1.col.Blend128(geom_rgba);

			int orig_tris = 0;
			int valid_tris = 0;

			//Dbg_Message("Number of verts: %d", num_verts);

			for (int idx = 2; idx < num_verts; idx++)
			{
				if (short_xyz)
				{
					NxPs2::dma::ConvertXYZToFloat(v2.pos, &(p_vert_array[idx * 4]), mesh_center);
				} else {
					NxPs2::dma::ConvertXYZToFloat(v2.pos, &(p_vert_array[idx * 4]));
				}
				NxPs2::dma::ConvertSTToFloat(v2.u, v2.v, &(p_uv_array[idx * 2]));
				v2.col = *((Image::RGBA *) &(p_rgb_array[idx]));
				v2.col.Blend128(geom_rgba);

				// Dont make triangle if vert has ADC bit set
				if (!(p_vert_array[(idx * 4) + 3] & 0x8000))		// if adc bit not set
				{
					orig_tris++;
					valid_tris++;

					// Push this tri onto the stack.
					triSubdivideStack.Push( &v0 );
					triSubdivideStack.Push( &v1 );
					triSubdivideStack.Push( &v2 );

					//Dbg_Message("Triangle (%f, %f, %f, %f) - (%f, %f, %f, %f) - (%f, %f, %f, %f)", v0.pos[X], v0.pos[Y], v0.pos[Z], v0.pos[W],
					//			 v1.pos[X], v1.pos[Y], v1.pos[Z], v1.pos[W], v2.pos[X], v2.pos[Y], v2.pos[Z], v2.pos[W]);
					//Dbg_Message("UVs (%f, %f) - (%f, %f) - (%f, %f)", v0.u, v0.v, v1.u, v1.v, v2.u, v2.v);
					//Dbg_Message("Colors (%d, %d, %d, %d) - (%d, %d, %d, %d) - (%d, %d, %d, %d)", v0.col.r, v0.col.g, v0.col.b, v0.col.a,
					//			 v1.col.r, v1.col.g, v1.col.b, v1.col.a, v2.col.r, v2.col.g, v2.col.b, v2.col.a);

					// Add extra triangles that we need
					valid_tris += get_num_extra_shatter_tris(shatterAreaTest, v0.pos, v1.pos, v2.pos);
				}

				v0 = v1;
				v1 = v2;
			}

			// Free DMA buffers
			delete [] p_vert_array;
			delete [] p_uv_array;
			delete [] p_rgb_array;

			if( valid_tris == 0 )
			{
				continue;
			}

			float newShatterArea = shatterAreaTest;

			// Make sure there is memory available and that we don't take it all
			int free_mem = Mem::Available();
			int needed_mem = (sNGPSShatterInstanceDetails::sQueryMemoryNeeded(valid_tris) * 3) / 2; 
			if (free_mem < needed_mem)
			{
				// Calculate how much bigger we need to make the area
				Dbg_Message("Old needed memory %d free memory %d for %d tris", needed_mem, free_mem, valid_tris);
				float mem_ratio = ((float) needed_mem) / ((float) free_mem);
				int iratio = (int) (mem_ratio + 0.99f);		// Round up
				iratio = (iratio + 3) & ~3;					// Round up to the nearest divisible by 4
				newShatterArea = newShatterArea * (iratio * iratio);	// And square the ratio

				Dbg_Message("mem ratio %f iratio %d", mem_ratio, iratio);

				const Mth::Vector *pos0, *pos1, *pos2;
				valid_tris = orig_tris;
				for (int idx = 0; idx < orig_tris; idx++)
				{
					pos0 = &(((const sNGPSVert *) triSubdivideStack.Peek((idx * 3) + 0))->pos);
					pos1 = &(((const sNGPSVert *) triSubdivideStack.Peek((idx * 3) + 1))->pos);
					pos2 = &(((const sNGPSVert *) triSubdivideStack.Peek((idx * 3) + 2))->pos);
				   
					// Add extra triangles that we need
					valid_tris += get_num_extra_shatter_tris(newShatterArea, *pos0, *pos1, *pos2);
				}
				needed_mem = (sNGPSShatterInstanceDetails::sQueryMemoryNeeded(valid_tris) * 3) / 2; 
				Dbg_Message("New needed memory %d free memory %d for %d tris", needed_mem, free_mem, valid_tris);

				// See if this one is smaller, otherwise, give up
				if (free_mem < needed_mem)
				{
					triSubdivideStack.Clear();
					continue;
				}

				Dbg_Message("Increased shatter area from %f to %f", shatterAreaTest, newShatterArea);
			}

			//Dbg_Message("Making %d triangles", valid_tris);

			// Create a tracking structure for this mesh.
			sNGPSShatterInstanceDetails *p_details		= new sNGPSShatterInstanceDetails( valid_tris, p_node );
			sNGPSVert	 				*p_write_vertex	= p_details->mp_vert_array;
			uint32						details_index	= 0;

#if PRINT_SHATTER_MEMORY
			Dbg_Message("Allocated %d bytes for shatter of %d tris", sNGPSShatterInstanceDetails::sQueryMemoryNeeded(valid_tris), valid_tris);
#endif

			Mth::Vector					spread_center	= shatterVelocity * -shatterSpreadFactor;
			float						base_speed		= shatterVelocity.Length();

			spread_center[X] += mesh_center[X];
			spread_center[Y] += mesh_center[Y];
			spread_center[Z] += mesh_center[Z];
			spread_center[W] = 1.0f;
			
			// Add the tracking structure to the table.
			p_shatter_details_table->PutItem((uint32)p_details, p_details );
			
			// Process-subdivide the entire stack.
			sNGPSVert *p_copy_vertex = p_write_vertex;
			while( subdivide_tri_stack( newShatterArea, &p_write_vertex, p_node ));

			// Copy the (possibly subdivided) vertex data over.
			while( p_copy_vertex < p_write_vertex )
			{
				Mth::Vector *p_vert0 = &((p_copy_vertex + 0)->pos);
				Mth::Vector *p_vert1 = &((p_copy_vertex + 1)->pos);
				Mth::Vector *p_vert2 = &((p_copy_vertex + 2)->pos);
				
				// Calculate position as the midpoint of the three vertices per poly.
				p_details->mp_positions[details_index][X] = ( p_vert0->GetX() + p_vert1->GetX() + p_vert2->GetX() ) * ( 1.0f / 3.0f );
				p_details->mp_positions[details_index][Y] = ( p_vert0->GetY() + p_vert1->GetY() + p_vert2->GetY() ) * ( 1.0f / 3.0f );
				p_details->mp_positions[details_index][Z] = ( p_vert0->GetZ() + p_vert1->GetZ() + p_vert2->GetZ() ) * ( 1.0f / 3.0f );
				p_details->mp_positions[details_index][W] = 1.0f;

				// Calculate the vector <velocity> back from the bounding box of the object. Then use this to figure the 'spread' of the
				// shards by calculating the vector from this position to the center of each shard.
				float speed = base_speed + ( base_speed * (( shatterVelocityVariance * rand() ) / RAND_MAX ));
				p_details->mp_velocities[details_index] = ( p_details->mp_positions[details_index] - spread_center ).Normalize( speed );

				Mth::Vector axis( -1.0f + ( 2.0f * (float)rand() / RAND_MAX ), -1.0f + ( 2.0f * (float)rand() / RAND_MAX ), -1.0f + ( 2.0f * (float)rand() / RAND_MAX ), 0.0f);
				axis.Normalize();
				p_details->mp_matrices[details_index].Ident();
				p_details->mp_matrices[details_index].Rotate( axis, 0.1f * ((float)rand() / RAND_MAX ));

				p_copy_vertex += 3;
				//p_copy_u += 3;
				//p_copy_v += 3;
						
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
	sNGPSShatterInstanceDetails *p_ps2_details = static_cast<sNGPSShatterInstanceDetails*>( p_details );
	
	// Load up initial three vertex pointers.
	sNGPSVert *p_v0	= p_ps2_details->mp_vert_array;
	sNGPSVert *p_v1	= p_ps2_details->mp_vert_array + 1;
	sNGPSVert *p_v2	= p_ps2_details->mp_vert_array + 2;
	
	for( int i = 0; i < p_details->m_num_triangles; ++i )
	{
		// To move the shatter pieces:
		// 1) subtract position from each vertex
		// 2) rotate
		// 3) update position with velocity
		// 4) add new position to each vertex

		// The matrix holds 3 vectors at once.
		Mth::Matrix m;
		m[X].Set( p_v0->pos[X] - p_details->mp_positions[i][X], p_v0->pos[Y] - p_details->mp_positions[i][Y], p_v0->pos[Z] - p_details->mp_positions[i][Z] );
		m[Y].Set( p_v1->pos[X] - p_details->mp_positions[i][X], p_v1->pos[Y] - p_details->mp_positions[i][Y], p_v1->pos[Z] - p_details->mp_positions[i][Z] );
		m[Z].Set( p_v2->pos[X] - p_details->mp_positions[i][X], p_v2->pos[Y] - p_details->mp_positions[i][Y], p_v2->pos[Z] - p_details->mp_positions[i][Z] );
         
		m[X].Rotate( p_details->mp_matrices[i] );
		m[Y].Rotate( p_details->mp_matrices[i] );
		m[Z].Rotate( p_details->mp_matrices[i] );

		// Update the position and velocity of the shatter piece, dealing with bouncing if necessary.
		p_details->UpdateParameters( i, framelength );
      
		m[X] += p_details->mp_positions[i]; 
		m[Y] += p_details->mp_positions[i]; 
		m[Z] += p_details->mp_positions[i];

		p_v0->pos[X] = m[X][X]; p_v0->pos[Y] = m[X][Y]; p_v0->pos[Z] = m[X][Z];
		p_v1->pos[X] = m[Y][X]; p_v1->pos[Y] = m[Y][Y]; p_v1->pos[Z] = m[Y][Z];
		p_v2->pos[X] = m[Z][X]; p_v2->pos[Y] = m[Z][Y]; p_v2->pos[Z] = m[Z][Z];

		p_v0 = p_v0 + 3;
		p_v1 = p_v1 + 3;
		p_v2 = p_v2 + 3;
	}
}



/******************************************************************************
 *
 * 
 *****************************************************************************/
void plat_shatter_render( sShatterInstanceDetails *p_details )
{
	//Dbg_Message("In plat_shatter_render()");
	sNGPSShatterInstanceDetails *p_ps2_details = static_cast<sNGPSShatterInstanceDetails*>( p_details );

	Dbg_Assert( p_ps2_details );

	if (p_ps2_details->m_num_triangles == 0)
		return;

	bool textured = (p_ps2_details->mp_tex_regs != NULL);
	if (textured)
	{
		NxPs2::sGroup *p_group = p_ps2_details->mp_geom_node->GetGroup();
		NxPs2::dma::SetList(p_group);
		p_group->Used[NxPs2::render::Field] = true;
		p_ps2_details->mp_geom_node->GetTexture()->m_render_count++;
	}

	Mth::Vector sort_pos = p_ps2_details->mp_vert_array[0].pos;			// just use 1st vert of 1st tri
	sort_pos *= NxPs2::render::WorldToCamera;
	sort_pos[2] = -1000.0f;
	NxPs2::dma::BeginTag(NxPs2::dma::cnt, *(uint32 *)&sort_pos[2]);		// z-sort key
	NxPs2::vif::BASE(NxPs2::vu1::Loc);
	NxPs2::vif::OFFSET(0);
	uint vu1_loc = NxPs2::vu1::Loc;
	NxPs2::vu1::Loc = 0;						// must do this as a relative prim for a sortable list...

	if (textured)
	{
		NxPs2::CImmediateMode::sTextureGroupInit(REL);
		NxPs2::CImmediateMode::sStartPolyDraw(p_ps2_details->mp_tex_regs, p_ps2_details->m_num_tex_regs, REL, false);
	}
	else
	{
		NxPs2::CImmediateMode::sStartPolyDraw(NULL, PackALPHA(0,1,0,1,0), REL, false);
	}

	// Render the triangles
	sNGPSVert *p_vert_array = p_ps2_details->mp_vert_array;
	for (int i = 0; i < p_ps2_details->m_num_triangles; i++, p_vert_array += 3)
	{
		if (textured)
		{
			NxPs2::CImmediateMode::sDrawTriUV(p_vert_array[0].pos, p_vert_array[1].pos, p_vert_array[2].pos,
											  p_vert_array[0].u, p_vert_array[0].v,
											  p_vert_array[1].u, p_vert_array[1].v,
											  p_vert_array[2].u, p_vert_array[2].v,
											  *((uint32 *) &p_vert_array[0].col),
											  *((uint32 *) &p_vert_array[1].col),
											  *((uint32 *) &p_vert_array[2].col),
											  REL);
		} else {
			NxPs2::CImmediateMode::sDrawTri(p_vert_array[0].pos, p_vert_array[1].pos, p_vert_array[2].pos,
											*((uint32 *) &p_vert_array[0].col),
											*((uint32 *) &p_vert_array[1].col),
											*((uint32 *) &p_vert_array[2].col),
											REL);
		}

// this may break if put back in...
//#if DRAW_DEBUG_LINES
//		Gfx::AddDebugLine( p_vert_array[0].pos, p_vert_array[1].pos, MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
//		Gfx::AddDebugLine( p_vert_array[1].pos, p_vert_array[2].pos, MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
//		Gfx::AddDebugLine( p_vert_array[2].pos, p_vert_array[0].pos, MAKE_RGB( 200, 0, 0 ), MAKE_RGB( 200, 0, 0 ), 1 );
//#endif
	}

	NxPs2::dma::EndTag();
	((uint16 *)NxPs2::dma::pTag)[1] |= NxPs2::vu1::Loc & 0x3FF;	// must write some code for doing this automatically
	NxPs2::vu1::Loc += vu1_loc;

	if (p_ps2_details->mp_geom_node->GetGroup()->flags & GROUPFLAG_SORT)
	{
		NxPs2::dma::Tag(NxPs2::dma::cnt,0,0);
		NxPs2::vif::NOP();
		NxPs2::vif::NOP();
		NxPs2::dma::SetList(NULL);
	}

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

void		CFog::s_plat_enable_fog(bool enable)
{
	NxPs2::render::EnableFog = enable;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFog::s_plat_set_fog_near_distance(float distance)
{
	NxPs2::render::FogNear = distance;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFog::s_plat_set_fog_exponent(float exponent)
{
	Dbg_Message("Stub: CFog::SetFogExponent()");
	//NxPs2::Fx::SetupFogPalette(*((unsigned int *) &m_rgba), exponent);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFog::s_plat_set_fog_rgba(Image::RGBA rgba)
{
	//NxPs2::Fx::SetupFogPalette(*((unsigned int *) &rgba), m_exponent);

	// Set alpha (and clamp between 0.0 and 1.0)
	float f_alpha = (float) rgba.a / 128.0f;
	f_alpha = Mth::Min(f_alpha, 1.0f);
	f_alpha = Mth::Max(f_alpha, 0.0f);
	NxPs2::render::FogAlpha = f_alpha;

	// Set color
	NxPs2::render::FogCol = *((uint32 *) &rgba) & 0xFFFFFF;	// mask out alpha
}

void		CFog::s_plat_set_fog_color( void )
{
	// Doesn't need to do anything on PS2.
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CFog::s_plat_fog_update( void )
{
}
	


} // Nx
