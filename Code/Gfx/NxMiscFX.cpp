#include <core/defines.h>

#include <gel/collision/collision.h>
#include <gel/collision/colltridata.h>
#include <gel/collision/movcollman.h>
#include <gel/collision/batchtricoll.h>
#include <sys/timer.h>
#include <sk/modules/frontend/frontend.h>
#include <gfx/nx.h>
#include <gfx/NxTexMan.h>
#include <gfx/NxViewMan.h>
#include <gfx/debuggfx.h>
#include <sys/replay/replay.h>

#include "NxMiscFX.h"


namespace Nx
{

Lst::HashTable< sScreenFlashDetails >		*p_screen_flash_details_table	= NULL;
Lst::HashTable< sSplatInstanceDetails >		*p_splat_details_table			= NULL;
Lst::HashTable< sSplatTrailInstanceDetails >*p_splat_trail_details_table	= NULL;
Lst::HashTable< sShatterInstanceDetails >	*p_shatter_details_table		= NULL;

static const float	DEFAULT_AREA_TEST			= 288.0f;
static const float	DEFAULT_VELOCITY_VARIANCE	= 0.0f;
static const float	DEFAULT_SPREAD_FACTOR		= 1.0f;
static const float	DEFAULT_LIFETIME			= 4.0f;
static const float	DEFAULT_BOUNCE				= -10000.0f;
static const float	DEFAULT_BOUNCE_AMPLITUDE	= 0.8f;

Mth::Vector			shatterVelocity;
float				shatterAreaTest			= DEFAULT_AREA_TEST * DEFAULT_AREA_TEST;
float				shatterVelocityVariance	= DEFAULT_VELOCITY_VARIANCE;
float				shatterSpreadFactor		= DEFAULT_SPREAD_FACTOR;
float				shatterLifetime			= DEFAULT_LIFETIME;
float				shatterBounce			= DEFAULT_BOUNCE;
float				shatterBounceAmplitude	= DEFAULT_BOUNCE_AMPLITUDE;
sTriSubdivideStack	triSubdivideStack;



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int sSplatInstanceDetails::GetOldestSplat( void )
{
	int oldest		= m_lifetimes[0];
	int oldest_idx	= 0;
	
	for( uint32 idx = 0; idx < SPLAT_POLYS_PER_MESH; ++idx )
	{
		if( m_lifetimes[idx] == 0 )
		{
			return idx;
		}
		else if( m_lifetimes[idx] < oldest )
		{
			oldest		= m_lifetimes[idx];
			oldest_idx	= idx;
		}
	}

	// If we get here there wasn't a 'dead' splat, so return the oldest.
	return oldest_idx;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sTriSubdivideStack::Reset( void )
{
	m_offset		= 0;
	m_block_size	= 0;

	memset( m_data, 0x03, TRI_SUBDIVIDE_STACK_SIZE );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sTriSubdivideStack::Clear( void )
{
	m_offset		= 0;
	m_block_size	= 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sTriSubdivideStack::Push( void *p_data )
{
	Dbg_Assert( m_offset + m_block_size < TRI_SUBDIVIDE_STACK_SIZE );
	
	memcpy( m_data + m_offset, p_data, m_block_size );
	m_offset += m_block_size;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sTriSubdivideStack::Pop( void* p_data )
{
	Dbg_Assert( m_offset >= m_block_size );
	
	m_offset -= m_block_size;
	memcpy( p_data, m_data + m_offset, m_block_size );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const void * sTriSubdivideStack::Peek( uint index )
{
	int offset = index * m_block_size;
	Dbg_MsgAssert( offset < m_offset, ("Index %d is beyond end offset %d", index, m_offset) );
	
	return m_data + offset;
}

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sShatterInstanceDetails::sShatterInstanceDetails( int num_tris )
{
	//Dbg_Message("Allocating %d bytes for position arrays of %d tris", (sizeof(Mth::Vector) * 2 + sizeof(Mth::Matrix)) * num_tris, num_tris);

	mp_positions		= new Mth::Vector[num_tris];
	mp_velocities		= new Mth::Vector[num_tris];
	mp_matrices			= new Mth::Matrix[num_tris];
	m_num_triangles		= num_tris;

	m_gravity			= 128.0f;
	m_lifetime			= shatterLifetime;
	m_bounce_level		= shatterBounce;
	m_bounce_amplitude	= shatterBounceAmplitude;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sShatterInstanceDetails::~sShatterInstanceDetails( void )
{
	delete [] mp_positions;
	delete [] mp_velocities;
	delete [] mp_matrices;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sShatterInstanceDetails::UpdateParameters( int index, float timestep )
{
	Dbg_Assert( index < m_num_triangles );
	
	mp_positions[index]	+= mp_velocities[index] * timestep;

	if(( mp_positions[index][Y] < m_bounce_level ) && ( mp_velocities[index][Y] < 0.0f ))
	{
		// Hit the floor. Bounce back up.
		mp_positions[index][Y]	= m_bounce_level + ( m_bounce_level - mp_positions[index][Y] );
		mp_velocities[index][Y]	= mp_velocities[index][Y] * -m_bounce_amplitude;

		// And figure a new rotation matrix.
		Mth::Vector axis( -1.0f + ( 2.0f * (float)rand() / RAND_MAX ), -1.0f + ( 2.0f * (float)rand() / RAND_MAX ), -1.0f + ( 2.0f * (float)rand() / RAND_MAX ));
		axis.Normalize();
		mp_matrices[index].Ident();
		mp_matrices[index].Rotate( axis, 0.1f * ((float)rand() / RAND_MAX ));
	}

	mp_velocities[index][Y]	-= m_gravity * timestep;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int	sShatterInstanceDetails::s_query_memory_needed(int num_tris)
{
	int size = sizeof(sShatterInstanceDetails);
	size += (2 * sizeof(Mth::Vector) + sizeof(Mth::Matrix)) * num_tris;

	return size;
}


/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void MiscFXInitialize( void )
{
	if( p_screen_flash_details_table == NULL )
	{
		p_screen_flash_details_table = new Lst::HashTable< sScreenFlashDetails >( 4 );
	}

	if( p_splat_details_table == NULL )
	{
		p_splat_details_table = new Lst::HashTable< sSplatInstanceDetails >( 4 );
	}

	if( p_splat_trail_details_table == NULL )
	{
		p_splat_trail_details_table = new Lst::HashTable< sSplatTrailInstanceDetails >( 4 );
	}
	
	if( p_shatter_details_table == NULL )
	{
		p_shatter_details_table = new Lst::HashTable< sShatterInstanceDetails >( 4 );
	}
	
	plat_texture_splat_initialize();
	plat_shatter_initialize();

	triSubdivideStack.Reset();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void MiscFXCleanup( void )
{
	// Can cleanup the screen flash details here, since they are not platform specific.
	p_screen_flash_details_table->IterateStart();
	sScreenFlashDetails *p_details = p_screen_flash_details_table->IterateNext();
	while( p_details )
	{
		sScreenFlashDetails *p_delete	= p_details;
		p_details						= p_screen_flash_details_table->IterateNext();

		p_screen_flash_details_table->FlushItem((uint32)p_delete );
		delete p_delete;
	}
	
	// Ditto for the tetxure splat trail details.
	KillAllTextureSplats();

	// Clean up the shatter details.
	plat_shatter_cleanup();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void KillAllTextureSplats( void )
{
	// This was split into a separate function because it can also be used after turning geometry off,
	// to ensure no splats are 'floating'.
	p_splat_trail_details_table->IterateStart();
	sSplatTrailInstanceDetails *p_trail_details = p_splat_trail_details_table->IterateNext();
	while( p_trail_details )
	{
		sSplatTrailInstanceDetails *p_delete	= p_trail_details;
		p_trail_details							= p_splat_trail_details_table->IterateNext();

		p_splat_trail_details_table->FlushItem((uint32)p_delete );
		delete p_delete;
	}
	
	// Call the platform specific cleanup function.
	plat_texture_splat_cleanup();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void AddScreenFlash( int viewport, Image::RGBA from, Image::RGBA to, float duration, float z, uint32 flags, const char *p_texture_name )
{
	Replay::WriteScreenFlash(viewport, from, to, duration, z, flags);
	
	// Ensure the supplied viewport is within bounds.
#ifdef __NOPT_ASSERT__
	int num_viewports = CViewportManager::sGetNumActiveViewports();
	Dbg_Assert( viewport < num_viewports );
#endif		// __NOPT_ASSERT__

	// Resolve the texture name if present.
	CTexture *p_texture = NULL;
	if( p_texture_name )
	{
	}

	// Store these details.
	sScreenFlashDetails *p_flash = new sScreenFlashDetails;
	p_flash->m_from		= from;
	p_flash->m_to		= to;
	p_flash->m_duration	= duration;
	p_flash->m_lifetime	= duration;
	p_flash->m_z		= z;
	p_flash->m_flags	= flags;
	p_flash->mp_texture	= p_texture;
	p_flash->m_viewport	= viewport;

	p_screen_flash_details_table->PutItem((uint32)p_flash, p_flash );

}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void ScreenFlashUpdate( void )
{
	float framelength = Tmr::FrameLength();

	if( p_screen_flash_details_table )
	{
		p_screen_flash_details_table->IterateStart();
		sScreenFlashDetails *p_details = p_screen_flash_details_table->IterateNext();
		while( p_details )
		{
			sScreenFlashDetails *p_delete = NULL;

			// Don't process if paused.
			if( !Mdl::FrontEnd::Instance()->GamePaused() || ( p_details->m_flags & Nx::SCREEN_FLASH_FLAG_IGNORE_PAUSE ))
			{
				p_details->m_lifetime -= framelength;
				if( p_details->m_lifetime > 0.0f )
				{
					// Calculate the current color.
					float mult				= p_details->m_lifetime / p_details->m_duration;
					p_details->m_current.r	= p_details->m_to.r + (int)(((float)p_details->m_from.r - (float)p_details->m_to.r ) * mult );
					p_details->m_current.g	= p_details->m_to.g + (int)(((float)p_details->m_from.g - (float)p_details->m_to.g ) * mult );
					p_details->m_current.b	= p_details->m_to.b + (int)(((float)p_details->m_from.b - (float)p_details->m_to.b ) * mult );
					p_details->m_current.a	= p_details->m_to.a + (int)(((float)p_details->m_from.a - (float)p_details->m_to.a ) * mult );
				}
				else
				{
					p_delete = p_details;
				}
			}	

			p_details = p_screen_flash_details_table->IterateNext();

			if( p_delete )
			{
				p_screen_flash_details_table->FlushItem((uint32)p_delete );
				delete p_delete;
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void ScreenFlashRender( int viewport, uint32 flags )
{
	p_screen_flash_details_table->IterateStart();
	sScreenFlashDetails *p_details = p_screen_flash_details_table->IterateNext();
	while( p_details )
	{
		if( viewport == p_details->m_viewport )
		{
			if(( flags & Nx::SCREEN_FLASH_FLAG_BEHIND_PANEL ) == ( p_details->m_flags & Nx::SCREEN_FLASH_FLAG_BEHIND_PANEL ))
			{
				plat_screen_flash_render( p_details );
			}
		}
		p_details = p_screen_flash_details_table->IterateNext();
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void TextureSplatUpdate( void )
{
	// Don't process if paused.
	if( Mdl::FrontEnd::Instance()->GamePaused() && !Replay::RunningReplay())
	{
		return;
	}	

	if( p_splat_details_table )
	{
		int framelength_in_ms = (int)( Tmr::FrameLength() * 1000.0f );
		
		p_splat_details_table->IterateStart();
		sSplatInstanceDetails *p_details = p_splat_details_table->IterateNext();
		while( p_details )
		{
			// Initialize to -1 to indicate no active splats so far.
			p_details->m_highest_active_splat = -1;
			
			for( int i = 0; i < SPLAT_POLYS_PER_MESH; ++i )
			{
				if( p_details->m_lifetimes[i] > 0 )
				{
					p_details->m_lifetimes[i] -= framelength_in_ms;
					if( p_details->m_lifetimes[i] <= 0 )
					{
						// Make sure to set lifetime back to exactly zero to indicate ready for re-use.
						p_details->m_lifetimes[i] = 0;

						// This splat has just 'expired'. Reset this poly.
						plat_texture_splat_reset_poly( p_details, i );
					}
					else
					{
						p_details->m_highest_active_splat = i;
					}
				}
			}
			p_details = p_splat_details_table->IterateNext();
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool TextureSplat( Mth::Vector& splat_start, Mth::Vector& splat_end, float size, float lifetime, const char *p_texture_name, uint32 trail )
{
	Replay::WriteTextureSplat(splat_start,splat_end,size,lifetime,p_texture_name,trail);
	
	// If this is a flagged as a trail splat, see if we have a trail instance already that matches.
	sSplatTrailInstanceDetails *p_trail_details = NULL;
	if( trail > 0 )
	{
		uint32 time = Tmr::GetTime();

		p_splat_trail_details_table->IterateStart();
		p_trail_details = p_splat_trail_details_table->IterateNext();
		while( p_trail_details )
		{
			sSplatTrailInstanceDetails *p_delete = NULL;

			if(( time < p_trail_details->m_last_pos_added_time ) || (( time - p_trail_details->m_last_pos_added_time ) > 500 ))
			{
				// This trail is over half a second old - remove it.
				p_delete = p_trail_details;
			}
			else
			{
				// This trail had a point added within the last half second, so check it for bone ID.
				if( p_trail_details->m_trail_id == trail )
				{
					// ID's match, so check it for proximity. Seems like fast moving reverts can cause successive calls to be up to 5.5 feet away.
					float sq_dist = ( splat_start - p_trail_details->m_last_pos ).LengthSqr();
					if( sq_dist < ( 66.0f * 66.0f ))
					{
						// This trail had a point added within the last half second that is within 3 feet of this point. Select this trail.
						break;
					}
				}
			}
			
			p_trail_details = p_splat_trail_details_table->IterateNext();

			if( p_delete )
			{
				p_splat_trail_details_table->FlushItem((uint32)p_delete );
				delete p_delete;
			}
		}

		// If there were no trail details, create a new instance and just return.
		if( p_trail_details == NULL )
		{
			p_trail_details = new sSplatTrailInstanceDetails;
			p_trail_details->m_trail_id				= trail;
			p_trail_details->m_last_pos				= splat_start;
			p_trail_details->m_last_pos_added_time	= time;
			p_splat_trail_details_table->PutItem((uint32)p_trail_details, p_trail_details );
			return true;
		}
	}
	
	// Convert the name string to a checksum.
	uint32 texture_checksum = Crc::GenerateCRCFromString( p_texture_name );
	
	// Obtain a pointer to the texture.
	Nx::CTexture *p_texture	= Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( texture_checksum );
	Dbg_MsgAssert( p_texture, ("Couldn't find texture %s in particle texture dictionary", p_texture_name) );

	Mth::Line is( splat_start, splat_end );

	// Make initial line bounding box.
	Mth::CBBox line_bbox( is.m_start );
	line_bbox.AddPoint( is.m_end );
	
	// Get a list of all collision sectors within all super sectors that the line intersects.
	// This is a very fast call, but likely to provide many collision sectors that are not actually within the
	// bounding box of the line.
	SSec::Manager	*ss_man				= Nx::CEngine::sGetNearestSuperSectorManager( is );
	Nx::CCollStatic	**pp_coll_obj_list	= ss_man->GetIntersectingCollSectors( is );

	// Can't create blood splat if no objects found.
	if( !pp_coll_obj_list )
	{
		return false;
	}

	// The sector table is used to pass a platform independent set of CSector pointers to the below p-line code.
	// Garrett: Also added static collision table, just in case CSector points to a CCollMulti
	#define SECTOR_TABLE_SIZE		127
	static CSector* sector_table[SECTOR_TABLE_SIZE + 1];
	static Nx::CCollStatic* collision_table[SECTOR_TABLE_SIZE + 1];

	int				sector_table_index = 0;
	Nx::CCollStatic *p_coll_obj;
	while(( p_coll_obj = *pp_coll_obj_list ))
	{
		if( p_coll_obj->GetGeometry())
		{
			// Determine whether this collision sector actually falls within the bounding box of our line.
			if( p_coll_obj->GetGeometry()->GetBBox().Intersect( line_bbox ))
			{
				uint32 checksum = p_coll_obj->GetChecksum();

				// From the checksum we can get the CSector (the renderable version) from the hash table of the scene.
				Nx::CSector *p_sector = Nx::CEngine::sGetSector( checksum );

				// Store the pointer in the table.
				Dbg_Assert( sector_table_index < SECTOR_TABLE_SIZE );
				collision_table[sector_table_index] = p_coll_obj;
				sector_table[sector_table_index++] = p_sector;
			}
			++pp_coll_obj_list;
		}
	}

	// Mark the end of the sector table with a NULL pointer.
	if( sector_table_index > 0 )
	{
		sector_table[sector_table_index] = NULL;
		collision_table[sector_table_index] = NULL;

		// Make sure we can can calculate an up vector.  If not, just move on.
		bool rv;
		if ((trail == 0) || (splat_start[X] != p_trail_details->m_last_pos[X]) || (splat_start[Z] != p_trail_details->m_last_pos[Z]))
		{
			rv = plat_texture_splat( sector_table, collision_table, splat_start, splat_end, size, lifetime, p_texture, p_trail_details );
		} else {
			rv = false;
		}

		// Update the trail details if present.
		if( p_trail_details )
		{
			p_trail_details->m_last_pos	= splat_start;
			p_trail_details->m_last_pos_added_time	= Tmr::GetTime();
		}

		return rv;
	}

	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void TextureSplatRender( void )
{

	
	plat_texture_splat_render();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void ShatterSetParams( Mth::Vector& velocity, float area_test, float velocity_variance, float spread_factor, float lifetime, float bounce, float bounce_amplitude )
{
	Replay::WriteShatterParams(velocity, area_test, velocity_variance, spread_factor, lifetime, bounce, bounce_amplitude);

	shatterVelocity			= velocity;
	shatterAreaTest			= ( area_test == 0.0f ) ? ( DEFAULT_AREA_TEST * DEFAULT_AREA_TEST ) : ( area_test * area_test );
	shatterVelocityVariance	= ( velocity_variance == 0.0f ) ? DEFAULT_VELOCITY_VARIANCE : velocity_variance;
	shatterSpreadFactor		= ( spread_factor == 0.0f ) ? DEFAULT_SPREAD_FACTOR : spread_factor;
	shatterLifetime			= ( lifetime == 0.0f ) ? DEFAULT_LIFETIME : lifetime;
	shatterBounce			= ( bounce == 0.0f ) ? DEFAULT_BOUNCE : bounce;
	shatterBounceAmplitude	= ( bounce_amplitude == 0.0f ) ? DEFAULT_BOUNCE_AMPLITUDE : bounce_amplitude;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Shatter( CGeom *p_geom )
{
	plat_shatter( p_geom );

	// After shattering the subdivision stack should be empty.
	Dbg_Assert( triSubdivideStack.IsEmpty());
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void ShatterUpdate( void )
{
	// Don't process if paused.
	if( Mdl::FrontEnd::Instance()->GamePaused() && !Replay::RunningReplay())
	{
		return;
	}	
	if (Replay::Paused())
	{
		return;
	}
		
	if( p_shatter_details_table )
	{
		float framelength = Tmr::FrameLength();
		
		p_shatter_details_table->IterateStart();
		sShatterInstanceDetails *p_details = p_shatter_details_table->IterateNext();
		while( p_details )
		{
			sShatterInstanceDetails *p_delete = NULL;
			
			p_details->m_lifetime -= framelength;
			if( p_details->m_lifetime <= 0.0f )
			{
				// Remove this entry from the table and destroy at the bottom of the loop.
				p_delete = p_details;
			}
			else
			{
				plat_shatter_update( p_details, framelength );
			}
			p_details = p_shatter_details_table->IterateNext();

			if( p_delete )
			{
				p_shatter_details_table->FlushItem((uint32)p_delete );
				delete p_delete;
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void ShatterRender( void )
{
	if( p_shatter_details_table )
	{
		p_shatter_details_table->IterateStart();
		sShatterInstanceDetails *p_details = p_shatter_details_table->IterateNext();
		while( p_details )
		{
			plat_shatter_render( p_details );
			p_details = p_shatter_details_table->IterateNext();
		}
	}
}


///////////////////////////////////////////////////////////////////
//
// FOG
//
///////////////////////////////////////////////////////////////////

bool		CFog::s_enabled = false;
float		CFog::s_near_distance = 3000.0f;
float		CFog::s_exponent = 10.0f;
Image::RGBA	CFog::s_rgba(0x60, 0x60, 0x80, 0x66);

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFog::sEnableFog(bool enable)
{
	s_enabled = enable;
	s_plat_enable_fog(enable);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFog::sSetFogNearDistance(float distance)
{
	s_near_distance = distance;
	s_plat_set_fog_near_distance(distance);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFog::sSetFogExponent(float exponent)
{
	s_exponent = exponent;
	s_plat_set_fog_exponent(exponent);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFog::sSetFogRGBA(Image::RGBA rgba)
{
	s_rgba = rgba;
	s_plat_set_fog_rgba(rgba);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFog::sSetFogColor( void )
{
	s_plat_set_fog_color();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFog::sFogUpdate( void )
{
	s_plat_fog_update();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CFog::sIsFogEnabled()
{
	return s_enabled;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float		CFog::sGetFogNearDistance()
{
	return s_near_distance;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float		CFog::sGetFogExponent()
{
	return s_exponent;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA	CFog::sGetFogRGBA()
{
	return s_rgba;
}

} // Nx
