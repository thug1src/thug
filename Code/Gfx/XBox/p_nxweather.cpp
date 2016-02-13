//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_NxWeather.cpp
//* OWNER:          Dave Cowling
//* CREATION DATE:  6/19/2003
//****************************************************************************

#include <core/defines.h>
#include "gfx/xbox/nx/render.h"

#include <gfx/NxTexMan.h>
#include <gfx/xbox/p_nxtexture.h>
#include "gfx/xbox/nx/mesh.h"
#include "gfx/xbox/p_nxweather.h"

#include <sk/modules/skate/skate.h>
#include <sk/objects/skater.h>

#include <sk/engine/feeler.h>
#include <engine/SuperSector.h>
#include "gfx/nx.h"

#include "gfx/xbox/nx/sprite.h"

#define FEELER_START			50000.0f
#define NUM_LINES_PER_BATCH		80
#define NUM_SPRITES_PER_BATCH	80

#define PRECISION_SHIFT			4

#define RENDER_DIST				16
#define SEQ_START				411		// Between 1 and 4095.
#define SEQ_MASK				0x0240

unsigned char grid_bytes[70*1024];

//1-3: 0x03
//1-7: 0x06
//1-15: 0x0C
//1-31: 0x14
//1-63: 0x30
//1-127: 0x60
//1-255: 0xB8
//1-511: 0x0110
//1-1023: 0x0240
//1-2047: 0x0500
//1-4095: 0x0CA0
//1-8191: 0x1B00
//1-16383: 0x3500
//1-32767: 0x6000
//1-65535: 0xB400
//0x00012000
//0x00020400
//0x00072000
//0x00090000
//0x00140000
//0x00300000
//0x00400000
//0x00D80000
//0x01200000
//0x03880000
//0x07200000
//0x09000000
//0x14000000
//0x32800000
//0x48000000
//0xA3000000


namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxWeather::CXboxWeather()
{
	mp_roof_height_index	= NULL;
	mp_rain_texture			= NULL;

	m_rain_blend			= NxXbox::GetBlendMode( CRCD( 0xa86285a1, "fixadd" )) | 0x80000000UL;
	m_splash_blend			= NxXbox::GetBlendMode( CRCD( 0xa86285a1, "fixadd" )) | 0x80000000UL;
	m_snow_blend			= NxXbox::GetBlendMode( CRCD( 0xa86285a1, "fixadd" )) | 0x80000000UL;

	m_rain_rate				= 0.0f;
	m_splash_rate			= 0.0f;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxWeather::~CXboxWeather()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxWeather::plat_update_grid( void )
{
	m_system_active = false;

	// Get super sector manager.
	SSec::Manager *ss_man;
	Mth::Line line;
	ss_man = Nx::CEngine::sGetNearestSuperSectorManager( line );		// Line is ignored, 1st manager is returned.
	if ( !ss_man ) return;

	// Calculate the size of the world in cels.
	int min_x = ( ((int)ss_man->GetWorldBBox()->GetMin()[X]) / WEATHER_CEL_SIZE ) - 1;
	int max_x = ( ((int)ss_man->GetWorldBBox()->GetMax()[X]) / WEATHER_CEL_SIZE ) + 1;
	int min_z = ( ((int)ss_man->GetWorldBBox()->GetMin()[Z]) / WEATHER_CEL_SIZE ) - 1;
	int max_z = ( ((int)ss_man->GetWorldBBox()->GetMax()[Z]) / WEATHER_CEL_SIZE ) + 1;

	// Define a maximum...
	if(( max_x - min_x ) > 350 )
	{
		int wid = ( max_x - min_x );
		int excess = ( wid - 350 );
		min_x += ( excess / 2 );
		max_x -= ( excess / 2 );
	}

	if(( max_z - min_z ) > 300 )
	{
		int wid = ( max_z - min_z );
		int excess = ( wid - 300 );
		min_z += ( excess / 2 );
		max_z -= ( excess / 2 );
	}

	// This is the actual width.
	m_width = ( max_x - min_x ) + 1;
	m_height = ( max_z - min_z ) + 1;

	m_min_x = (float)( min_x * WEATHER_CEL_SIZE );
	m_min_z = (float)( min_z * WEATHER_CEL_SIZE );

	// Temporary buffer for the raw array.
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	unsigned char * p8 = new unsigned char[m_width*m_height];
	Mem::Manager::sHandle().PopContext();

	// Go through and get the height for each cel corner.
	CFeeler feeler;
	int num_heights = 0;
	int xx, zz;
	for ( zz = min_z; zz <= max_z; zz++ )
	{
		for ( xx = min_x; xx <= max_x; xx++ )
		{
			// The cel to fill.
			int cel = ( ( zz - min_z ) * m_width ) + ( xx - min_x );

			// The position to check.
			Mth::Vector pos;
			pos[X] = (float)( xx * WEATHER_CEL_SIZE );
			pos[Y] = FEELER_START;
			pos[Z] = (float)( zz * WEATHER_CEL_SIZE );
			pos[W] = 1.0f;

			feeler.SetStart( pos );
			pos[Y] = -FEELER_START;
			feeler.SetEnd( pos );

			// Get the y position.
			sint32 y;
			if ( feeler.GetCollision( false, false ) )		// No movables, nearest collision.
			{
				y = (sint32)( feeler.GetPoint()[Y] * SUB_INCH_PRECISION );
			}
			else
			{
				y = (sint32)( FEELER_START * SUB_INCH_PRECISION );
			}

			// See if a close enough y pos already exists.
			int found = -1;
			for ( int lp = 0; lp < num_heights; lp++ )
			{
				if ( abs( ( m_roof_height[lp] - y ) ) < HEIGHT_TOLERANCE )
				{
					found = lp;
					break;
				}
			}

			// Fill in the cel.
			if ( found != -1 )
			{
				// Existing height.
				p8[cel] = found;
			}
			else
			{
				// New height.
				p8[cel] = num_heights;
				m_roof_height[num_heights] = y;
				num_heights++;
			}
		}
	}

	// Work out highest height for each cel.
	for ( zz = min_z; zz <= max_z; zz++ )
	{
		for ( xx = min_x; xx <= max_x; xx++ )
		{
			// The cel to fill.
			int cel = ( ( zz - min_z ) * m_width ) + ( xx - min_x );
			int celx = ( ( zz - min_z ) * m_width ) + ( ( xx + 1 ) - min_x );
			int celz = ( ( ( zz + 1 ) - min_z ) * m_width ) + ( xx - min_x );
			int celxz = ( ( ( zz + 1 ) - min_z ) * m_width ) + ( ( xx + 1 ) - min_x );

			if ( m_roof_height[p8[celx]] > m_roof_height[p8[cel]] ) p8[cel] = p8[celx];
			if ( m_roof_height[p8[celz]] > m_roof_height[p8[cel]] ) p8[cel] = p8[celz];
			if ( m_roof_height[p8[celxz]] > m_roof_height[p8[cel]] ) p8[cel] = p8[celxz];
		}
	}

	// Create a sparse array.
	mp_roof_row = (sRowEntry *)grid_bytes;
	mp_roof_height_index = (unsigned char *)&mp_roof_row[m_height];

	// 0 = offset
	// 1 = width
	// 2 = index

	unsigned short index = 0;
	for ( zz = 0; zz <= m_height; zz++ )
	{
		unsigned short start = 0;
		unsigned short end = m_width - 1;

		// Scan to find the start.
		bool start_set = false;
		for ( xx = 0; xx < m_width; xx++ )
		{
			int cel = ( zz * m_width ) + xx;

			if ( m_roof_height[p8[cel]] != (sint32)( FEELER_START * SUB_INCH_PRECISION ) )
			{
				if ( !start_set )
				{
					// Set start value.
					start = xx;
					start_set = true;
				}
				else
				{
					// Set end value.
					end = xx;
				}

			}
		}

		// Copy data & set row entry.
		if ( start < end )
		{
			mp_roof_row[zz].start = start;
			mp_roof_row[zz].end = end;
			mp_roof_row[zz].index = index;
			for ( xx = start; xx <= end ; xx++ )
			{
				int cel = ( zz * m_width ) + xx;

				mp_roof_height_index[index] = p8[cel];
				index++;
			}
		}
		else
		{
			// Row doesn't exist.
			mp_roof_row[zz].start = 16384;
			mp_roof_row[zz].end = 0;
			mp_roof_row[zz].index = 0;
		}
	}

	delete p8;

	int new_size = ( m_height * 6 ) + index;

	Dbg_Message( "Grid Size: Old: %d New: %d Num Heights: %d\n", ( m_width * m_height ), new_size, num_heights );

	// Set all drip time counters to 255.
	// 255 means the drop won't be rendered.
	// Setting to anything other than 255 will mean that it will increment to 255 and stop.
	for ( int lp = 0; lp < ( DROP_SIZE * DROP_SIZE * DROP_LAYERS ); lp++ )
	{
		m_drop_time[lp] = 255;
		m_x_offset[lp] = Mth::Rnd( 255 );
		m_z_offset[lp] = Mth::Rnd( 255 );
	}
	m_active_drops = 0;

	m_seq = SEQ_START;

	// Get the texture.
	Nx::CTexture*		p_texture;
	Nx::CXboxTexture*	p_xbox_texture;
	mp_rain_texture = NULL;

	// Set Rain Texture.
	p_texture		= Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( CRCD( 0x45c7eb0f,"splash" ));
	p_xbox_texture	= static_cast<Nx::CXboxTexture*>( p_texture );
	if( p_xbox_texture )
	{
		mp_rain_texture = p_xbox_texture->GetEngineTexture();
	}

	// Set Snow Texture.
	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( CRCD( 0xc97c5a4c,"snow" ));
	p_xbox_texture = static_cast<Nx::CXboxTexture*>( p_texture );
	if( p_xbox_texture )
	{
		mp_snow_texture = p_xbox_texture->GetEngineTexture();
	}

	m_system_active = true;

	// Zero out the splashes.
	for( int sp = 0; sp < NUM_SPLASH_ACTIVE; sp++ )
	{
		m_splash_current_life[sp] = 0;
	}
	m_current_splash = 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxWeather::plat_process( float delta_time )
{
	if( !m_system_active ) return;

	if( m_raining )
	{
		// It's raining.
		float rate = m_rain_drops_per_frame;
		if( rate > (float)(( DROP_SIZE * DROP_SIZE * DROP_LAYERS ) / m_rain_frames ))
		{
			rate = (float)( ( DROP_SIZE * DROP_SIZE * DROP_LAYERS ) / m_rain_frames );
		}

		float last = m_rain_rate;
		m_rain_rate += rate;
		int new_drops = (int)m_rain_rate - (int)last;

		for( int lp = 0; lp < new_drops; lp++ )
		{
			// If this cel is not inactive, we caught up with ourselves.
			if ( m_drop_time[m_seq] != 255 ) break;

			// Setup the drop.
			m_drop_time[m_seq] = ( 254 - m_rain_frames );
			m_x_offset[m_seq] = Mth::Rnd( 255 );
			m_z_offset[m_seq] = Mth::Rnd( 255 );
			m_active_drops++;

			// Calculate next sequence value. Notice hack to get entry 0 into the sequence.
			switch ( m_seq )
			{
				case SEQ_START:
					m_seq = 0;
					break;
				case 0:
					m_seq = SEQ_START;
					// Fall through to default case...
				default:
					if ( m_seq & 1 )
					{
						m_seq = ( m_seq >> 1 ) ^ SEQ_MASK;
					}
					else
					{
						m_seq = ( m_seq >> 1 );
					}
					break;
			}
		}
	}
	else
	{
		// It's snowing.
		float rate = m_snow_flakes_per_frame;
		if ( rate > (float)( ( DROP_SIZE * DROP_SIZE * DROP_LAYERS ) / m_snow_frames ) ) rate = (float)( ( DROP_SIZE * DROP_SIZE * DROP_LAYERS ) / m_snow_frames );

		float last = m_snow_rate;
		m_snow_rate += rate;
		int new_drops = (int)m_snow_rate - (int)last;

		for ( int lp = 0; lp < new_drops; lp++ )
		{
			// If this cel is not inactive, we caught up with ourselves.
			if ( m_drop_time[m_seq] != 255 ) break;

			// Setup the drop.
			m_drop_time[m_seq] = ( 254 - m_snow_frames );
			m_x_offset[m_seq] = Mth::Rnd( 255 );
			m_z_offset[m_seq] = Mth::Rnd( 255 );
			m_active_drops++;

			// Calculate next sequence value. Notice hack to get entry 0 into the sequence.
			switch ( m_seq )
			{
				case SEQ_START:
				{
					m_seq = 0;
					break;
				}
				case 0:
				{
					m_seq = SEQ_START;
					// Fall through to default case...
				}
				default:
				{
					if ( m_seq & 1 )
					{
						m_seq = ( m_seq >> 1 ) ^ SEQ_MASK;
					}
					else
					{
						m_seq = ( m_seq >> 1 );
					}
					break;
				}
			}
		}
	}
}



inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxWeather::plat_render_snow( float skx, float skz )
{
	// Early out if no drops to draw.
	if( !m_active_drops ) return;

	// Get skater position.
	int x = (int)(( skx - m_min_x ) / WEATHER_CEL_SIZE );
	int z = (int)(( skz - m_min_z ) / WEATHER_CEL_SIZE );

	// Calculate area to render.
	int sx = x - RENDER_DIST;
	int ex = x + DROP_SIZE;	//RENDER_DIST;
	int sz = z - RENDER_DIST;
	int ez = z + DROP_SIZE;	//RENDER_DIST;

	// Clip z values.
	if( ez < 0 ) return;
	if( sz > ( m_height - 1 )) return;

	// Pointsprites use texture stage 3 for some reason...
	NxXbox::set_blend_mode( m_snow_blend );
	NxXbox::set_render_state( RS_ALPHACUTOFF, 0 );
	NxXbox::set_render_state( RS_UVADDRESSMODE3, 0x00000000UL );
	mp_snow_texture->Set( 3 );

	// Set up point sprite rendering.
	D3DDevice_SetRenderState( D3DRS_POINTSPRITEENABLE,	TRUE );
	D3DDevice_SetRenderState( D3DRS_POINTSCALEENABLE,	TRUE );
	D3DDevice_SetRenderState( D3DRS_POINTSIZE,			FtoDW( m_snow_size ));
	D3DDevice_SetRenderState( D3DRS_POINTSCALE_A,		FtoDW( 0.00f ));
	D3DDevice_SetRenderState( D3DRS_POINTSCALE_B,		FtoDW( 0.00f ));
	D3DDevice_SetRenderState( D3DRS_POINTSCALE_C,		FtoDW( 1.00f ));

	NxXbox::set_pixel_shader( PixelShaderPointSprite );
	NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_DIFFUSE );

	float minx = m_min_x;
	float minz = m_min_z;
	
	// Calculate drop height list.
	int lp;
	float y_off[256];
	for( lp = ( 254 - m_snow_frames ); lp < 256; lp++ )
	{
		y_off[lp] = m_snow_height - (((float)(( lp - 254 ) + m_snow_frames ) / (float)m_snow_frames ) * m_snow_height );
	}

	// Calculate xz offset list.
	float xz_off[256];
	for( lp = 0; lp < 256; lp++ )
	{
		xz_off[lp] = ((float)lp * (float)( WEATHER_CEL_SIZE / 2 )) / 255.0f;
	}

	// Calculate sine wave.
	float sin_off[256];

	for( lp = 0; lp < 256; lp++ )
	{
		sin_off[lp] = sinf( Mth::PI * 2.0f * ((float)lp / 256.0f )) * ( WEATHER_CEL_SIZE / 2 );
	}

	// Get Xbox format color.
	uint32 snow_color = ((uint32)m_snow_color.a << 24 ) | ((uint32)m_snow_color.r << 16 ) | ((uint32)m_snow_color.g << 8 ) | ((uint32)m_snow_color.b << 0 );

	int	flakes = 0;

	DWORD* p_push				= NULL;
	DWORD* p_push_encode_fixup	= NULL;
	uint32 dwords_written		= 0;
	uint32 total_dwords_written	= 2;

	for( int lzz = sz; lzz <= ez; lzz++ )
	{
		int zz = ( lzz < 0 ) ? 0 : ( lzz > ( m_height - 1 ) ) ? ( m_height - 1 ) : lzz;

		if( mp_roof_row[zz].start == 16384 ) continue;

		float vx = ((float)sx * (float)WEATHER_CEL_SIZE ) + minx;
		float vz = ((float)zz * (float)WEATHER_CEL_SIZE ) + minz;

		int cel = mp_roof_row[zz].index + ( sx - mp_roof_row[zz].start );

		int drop_cel_z = (( zz & ( DROP_SIZE - 1 )) << DROP_SIZE_SHIFT );

		for( int lxx = sx; lxx <= ex; lxx++, cel++ )
		{
			int xx = ( lxx > mp_roof_row[zz].start ) ? ( ( lxx < mp_roof_row[zz].end ) ? lxx : mp_roof_row[zz].end ) : mp_roof_row[zz].start;

			// Get the current drop value. Skip this one if it's inactive.
			int drop_cel = drop_cel_z + ( xx & ( DROP_SIZE - 1 ));

			vx += (float)WEATHER_CEL_SIZE;
			float vy = (float)( m_roof_height[mp_roof_height_index[cel]] >> PRECISION_SHIFT );

			for( int d = 0; d < DROP_LAYERS; d++, drop_cel += ( DROP_SIZE * DROP_SIZE ))
			{
				if( m_drop_time[drop_cel] == 255 )
					continue;

				// Create the position for rendering.
				float v0x = vx + xz_off[m_x_offset[drop_cel]] + sin_off[(m_z_offset[drop_cel]+m_drop_time[drop_cel])&255];
				float v0y = vy + y_off[m_drop_time[drop_cel]] + m_snow_size;
				float v0z = vz + xz_off[m_z_offset[drop_cel]] + sin_off[(m_x_offset[drop_cel]+m_drop_time[drop_cel])&255];

				if( flakes == 0 )
				{
					flakes = NUM_SPRITES_PER_BATCH;
				}

				// Grab the push buffer if we don't have it yet.
				if( p_push == NULL )
				{
					// Grab 32k of push buffer.	
					p_push = D3DDevice_BeginPush( 32 * 1024 / 4 );

					// Note that p_push is returned as a pointer to write-combined memory. Writes to write-combined memory should be
					// consecutive and in increasing order. Reads should be avoided. Additionally, any CPU reads from memory or the
					// L2 cache can force expensive partial flushes of the 32-byte write-combine cache.
					p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
					p_push[1]	= D3DPT_POINTLIST;
					p_push		+= 2;

					// NOTE: A maximum of 2047 DWORDs can be specified to D3DPUSH_ENCODE. If there is more than 2047 DWORDs of vertex
					// data, simply split the data into multiple D3DPUSH_ENCODE( D3DPUSH_INLINE_ARRAY ) sections.
					// As yet, we don't know how many dwords we will write, so save a pointer to the current push buffer location
					// so we can fix up this value at the end.
					p_push_encode_fixup = p_push;
					++p_push;
				}

				p_push[0]				= *((uint32*)&v0x );
				p_push[1]				= *((uint32*)&v0y );
				p_push[2]				= *((uint32*)&v0z );
				p_push[3]				= snow_color;

				p_push					+= 4;
				dwords_written			+= 4;
				total_dwords_written	+= 4;

				// Check for hitting the max dwords for this encode block, in which case we need to end this block
				// and start another one.
				if( dwords_written >= 2044 )
				{
					// Now we know exactly how many dwords written, fix-up our earlier entry.
					p_push_encode_fixup[0]	= D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, dwords_written );

					// Are we approaching the end of this push buffer? If so, close it out and start a new one.
					if( total_dwords_written > 6000 )
					{
						// End the push buffer.
						total_dwords_written	= 0;
						p_push[0]				= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
						p_push[1]				= 0;
						p_push					+= 2;
						D3DDevice_EndPush( p_push );

						p_push					= NULL;
					}
					else
					{
						// As yet, we don't know how many dwords we will write, so save a pointer to the current push buffer location
						// so we can fix up this value at the end.
						p_push_encode_fixup		= p_push;
						++p_push;
					}

					dwords_written = 0;
				}

				flakes--;
			}
		}
	}

	if( p_push )
	{
		// Now we know exactly how many dwords written, fix-up our earlier entry.
		p_push_encode_fixup[0] = D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, dwords_written );

		// End the push buffer.
		p_push[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
		p_push[1] = 0;
		p_push += 2;
		D3DDevice_EndPush( p_push );
	}

	// Restore render states.
	D3DDevice_SetRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxWeather::plat_render_rain( float skx, float skz )
{
	// Early out if no drops to draw.
	if ( !m_active_drops ) return;

	// Get skater position.
	int x = (int)(( skx - m_min_x ) / WEATHER_CEL_SIZE );
	int z = (int)(( skz - m_min_z ) / WEATHER_CEL_SIZE );

	// Calculate area to render.
	int sx = x - RENDER_DIST;
	int ex = x + DROP_SIZE;	//RENDER_DIST;
	int sz = z - RENDER_DIST;
	int ez = z + DROP_SIZE;	//RENDER_DIST;

	// Clip z values.
	if ( ez < 0 ) return;
	if ( sz > ( m_height - 1 ) ) return;

	// Get Xbox format top and bottom color.
	uint32 top_color = ((uint32)m_rain_top_color.a << 24 ) | ((uint32)m_rain_top_color.r << 16 ) | ((uint32)m_rain_top_color.g << 8 ) | ((uint32)m_rain_top_color.b << 0 );
	uint32 bot_color = ((uint32)m_rain_bottom_color.a << 24 ) | ((uint32)m_rain_bottom_color.r << 16 ) | ((uint32)m_rain_bottom_color.g << 8 ) | ((uint32)m_rain_bottom_color.b << 0 );

	// Deal with FIXED_BRIGHTEN mode, which doesn't work.
	if(( m_rain_blend & NxXbox::sMaterial::BLEND_MODE_MASK ) == NxXbox::vBLEND_MODE_BRIGHTEN_FIXED )
	{
		set_blend_mode( NxXbox::vBLEND_MODE_BRIGHTEN );

		top_color = ( top_color & 0x00FFFFFFUL ) | ( m_rain_blend & 0xFF000000UL );
		bot_color = ( bot_color & 0x00FFFFFFUL ) | ( m_rain_blend & 0xFF000000UL );
	}
	else
	{
		NxXbox::set_blend_mode( m_rain_blend );
	}

	NxXbox::set_render_state( RS_ALPHACUTOFF, 0 );

	NxXbox::set_pixel_shader( PixelShader5 );
	NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_DIFFUSE );

	sint32 minx		= (sint32)( m_min_x * SUB_INCH_PRECISION );
	sint32 minz		= (sint32)( m_min_z * SUB_INCH_PRECISION );
	sint32 rlength	= (sint32)( m_rain_length * SUB_INCH_PRECISION );
	
	// Calculate drop height list.
	int lp;
	sint32 y_off[256];

	for( lp = ( 254 - m_rain_frames ); lp < 256; lp++ )
	{
		y_off[lp] = (sint32)(( m_rain_height - (((float)(( lp - 254 ) + m_rain_frames ) / (float)m_rain_frames ) * m_rain_height )) * SUB_INCH_PRECISION );
	}

	// Calculate xz offset list.
	sint32 xz_off[256];
	for( lp = 0; lp < 256; lp++ )
	{
		xz_off[lp] = (sint32)((((float)lp * (float)WEATHER_CEL_SIZE * SUB_INCH_PRECISION ) / 255.0f ));
	}

	int lines = 0;

	DWORD* p_push				= NULL;
	DWORD* p_push_encode_fixup	= NULL;
	uint32 dwords_written		= 0;
	uint32 total_dwords_written	= 2;

	for ( int lzz = sz; lzz <= ez; lzz++ )
	{
		int zz = ( lzz < 0 ) ? 0 : ( lzz > ( m_height - 1 ) ) ? ( m_height - 1 ) : lzz;

		if ( mp_roof_row[zz].start == 16384 ) continue;

		sint32 vx = ( (sint32)sx << ( WEATHER_CEL_SIZE_SHIFT + PRECISION_SHIFT ) ) + minx;
		sint32 vz = ( (sint32)zz << ( WEATHER_CEL_SIZE_SHIFT + PRECISION_SHIFT ) ) + minz;

		int cel = mp_roof_row[zz].index + ( sx - mp_roof_row[zz].start );

		int drop_cel_z = ( ( zz & ( DROP_SIZE - 1 ) ) << DROP_SIZE_SHIFT );

		for ( int lxx = sx; lxx <= ex; lxx++, cel++ )
		{
			int xx = ( lxx > mp_roof_row[zz].start ) ? ( ( lxx < mp_roof_row[zz].end ) ? lxx : mp_roof_row[zz].end ) : mp_roof_row[zz].start;

			// Get the current drop value. Skip this one if it's inactive.
			int drop_cel = drop_cel_z + ( xx & ( DROP_SIZE - 1 ) );

			vx += ( WEATHER_CEL_SIZE << PRECISION_SHIFT );
			sint32 vy = m_roof_height[mp_roof_height_index[cel]];

			for ( int d = 0; d < DROP_LAYERS; d++, drop_cel += ( DROP_SIZE * DROP_SIZE ) )
			{
				if ( m_drop_time[drop_cel] == 255 )
					continue;

				// Create the position for rendering.
				float v0x = ( vx + xz_off[m_x_offset[drop_cel]] ) * ( 1.0f / (float)( 1 << PRECISION_SHIFT ));
				float v0y = ( vy + y_off[m_drop_time[drop_cel]] ) * ( 1.0f / (float)( 1 << PRECISION_SHIFT ));
				float v0z = ( vz + xz_off[m_z_offset[drop_cel]] ) * ( 1.0f / (float)( 1 << PRECISION_SHIFT ));
				float v1y = v0y + ( rlength * ( 1.0f / (float)( 1 << PRECISION_SHIFT )));

				if ( lines == 0 )
				{
					lines = NUM_LINES_PER_BATCH;
				}

				// Grab the push buffer if we don't have it yet.
				if( p_push == NULL )
				{
					// Grab 32k of push buffer.	
					p_push = D3DDevice_BeginPush( 32 * 1024 / 4 );

					// Note that p_push is returned as a pointer to write-combined memory. Writes to write-combined memory should be
					// consecutive and in increasing order. Reads should be avoided. Additionally, any CPU reads from memory or the
					// L2 cache can force expensive partial flushes of the 32-byte write-combine cache.
					p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
					p_push[1]	= D3DPT_LINELIST;
					p_push		+= 2;

					// NOTE: A maximum of 2047 DWORDs can be specified to D3DPUSH_ENCODE. If there is more than 2047 DWORDs of vertex
					// data, simply split the data into multiple D3DPUSH_ENCODE( D3DPUSH_INLINE_ARRAY ) sections.
					// As yet, we don't know how many dwords we will write, so save a pointer to the current push buffer location
					// so we can fix up this value at the end.
					p_push_encode_fixup = p_push;
					++p_push;
				}

				p_push[0] = *((DWORD*)&v0x);
				p_push[1] = *((DWORD*)&v0y);
				p_push[2] = *((DWORD*)&v0z);
				p_push[3] = bot_color;

				p_push[4] = *((DWORD*)&v0x);
				p_push[5] = *((DWORD*)&v1y);
				p_push[6] = *((DWORD*)&v0z);
				p_push[7] = top_color;

				p_push					+= 8;
				dwords_written			+= 8;
				total_dwords_written	+= 8;

				// Check for hitting the max dwords for this encode block, in which case we need to end this block
				// and start another one.
				if( dwords_written >= 2040 )
				{
					// Now we know exactly how many dwords written, fix-up our earlier entry.
					p_push_encode_fixup[0]		= D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, dwords_written );

					// Are we approaching the end of this push buffer? If so, close it out and start a new one.
					if( total_dwords_written > 6000 )
					{
						// End the push buffer.
						total_dwords_written	= 0;
						p_push[0]				= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
						p_push[1]				= 0;
						p_push					+= 2;
						D3DDevice_EndPush( p_push );

						p_push					= NULL;
					}
					else
					{
						// As yet, we don't know how many dwords we will write, so save a pointer to the current push buffer location
						// so we can fix up this value at the end.
						p_push_encode_fixup		= p_push;
						++p_push;
					}
					dwords_written = 0;
				}
				lines--;
			}
		}
	}

	if( p_push )
	{
		// Now we know exactly how many dwords written, fix-up our earlier entry.
		p_push_encode_fixup[0] = D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, dwords_written );

		// End the push buffer.
		p_push[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
		p_push[1] = 0;
		p_push += 2;
		D3DDevice_EndPush( p_push );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxWeather::plat_render_splashes( float skx, float skz )
{
	// Create a new splash if required.
	float last = m_splash_rate;
	m_splash_rate += m_splash_per_frame;
	int new_splashes = (int)m_splash_rate - (int)last;

	if ( new_splashes )
	{
		CFeeler feeler;
		Mth::Vector pos;

		pos[X] = NxXbox::EngineGlobals.cam_position[X];
		pos[Y] = FEELER_START;
		pos[Z] = NxXbox::EngineGlobals.cam_position[Z];
		pos[W] = 1.0f;

		Mth::Vector dir;
		dir[X] = skx - NxXbox::EngineGlobals.cam_position[X];
		dir[Y] = 0.0f;
		dir[Z] = skz - NxXbox::EngineGlobals.cam_position[Z];
		dir[W] = 1.0f;
		dir.Normalize();

		// Add distance.
		float r1 = (float)Mth::Rnd( 32768 ) / 32768.0f;
		pos		+= ( dir * ((float)WEATHER_CEL_SIZE * RENDER_DIST * r1 ));

		// Add lateral.
		Mth::Vector lat( dir[Z], 0.0f, -dir[X], 1.0f );

		float r2 = 1.0f - ( (float)Mth::Rnd( 32768 ) / 32768.0f );
		if ( m_current_splash & 1 )
		{
			pos += ( lat * ( (float)WEATHER_CEL_SIZE * RENDER_DIST * r1 * r2 ) );
		}
		else
		{
			pos += ( lat * ( (float)WEATHER_CEL_SIZE * RENDER_DIST * r1 * -r2 ) );
		}

		feeler.SetStart( pos );
		pos[Y] = -FEELER_START;
		feeler.SetEnd( pos );

		// Get the y position.
		float y;
		if ( feeler.GetCollision( false, false ) )		// No movables, nearest collision.
		{
			y = feeler.GetPoint()[Y];
		}
		else
		{
			y = FEELER_START;
		}

		m_splash_x[m_current_splash]			= pos[X];
		m_splash_y[m_current_splash]			= y + 4.0f;	// Move up slightly.
		m_splash_z[m_current_splash]			= pos[Z];
		m_splash_current_life[m_current_splash]	= m_splash_life;

		m_current_splash++;
		m_current_splash %= NUM_SPLASH_ACTIVE;
	}
	
	// Render the splashes.
	NxXbox::set_blend_mode( m_splash_blend );
	NxXbox::set_render_state( RS_ALPHACUTOFF, 0 );
	NxXbox::set_render_state( RS_UVADDRESSMODE0, 0x00000000UL );
	mp_rain_texture->Set( 0 );

	NxXbox::set_pixel_shader( PixelShader4 );
	NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ));

	uint32 dword_count		= 24 * NUM_SPLASH_ACTIVE;
	uint32 dwords_written	= 0;
	Dbg_Assert( dword_count < 2048 );

	// Obtain push buffer lock.
	// The additional number (+5 is minimum) is to reserve enough overhead for the encoding parameters. It can safely be more, but no less.
	DWORD* p_push				= D3DDevice_BeginPush( dword_count + 16 );
	DWORD* p_push_encode_fixup	= NULL;

	// Note that p_push is returned as a pointer to write-combined memory. Writes to write-combined memory should be
	// consecutive and in increasing order. Reads should be avoided. Additionally, any CPU reads from memory or the
	// L2 cache can force expensive partial flushes of the 32-byte write-combine cache.
	p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
	p_push[1]	= D3DPT_QUADLIST;
	p_push		+= 2;

	// NOTE: A maximum of 2047 DWORDs can be specified to D3DPUSH_ENCODE. If there is more than 2047 DWORDs of vertex
	// data, simply split the data into multiple D3DPUSH_ENCODE( D3DPUSH_INLINE_ARRAY ) sections.
	// As yet, we don't know how many dwords we will write, so save a pointer to the current push buffer location
	// so we can fix up this value at the end.
	p_push_encode_fixup = p_push;
	++p_push;

	for ( int spl = 0; spl < NUM_SPLASH_ACTIVE; spl++ )
	{
		if( m_splash_current_life[spl] == 0 )
			continue;

		// Interpolate color (and switch to Xbox format at the same time).
		Image::RGBA	icol;
		icol.b		= (uint8)(((int)m_splash_color.r * m_splash_current_life[spl] ) / m_splash_life );
		icol.g		= (uint8)(((int)m_splash_color.g * m_splash_current_life[spl] ) / m_splash_life );
		icol.r		= (uint8)(((int)m_splash_color.b * m_splash_current_life[spl] ) / m_splash_life );
		icol.a		= (uint8)(((int)m_splash_color.a * m_splash_current_life[spl] ) / m_splash_life );

		// Draw splashes...
		float vx0	= m_splash_x[spl] - ( m_splash_size * 0.5f );
		float vx1	= m_splash_x[spl] + ( m_splash_size * 0.5f );
		float vy	= m_splash_y[spl];
		float vz0	= m_splash_z[spl] - ( m_splash_size * 0.5f );
		float vz1	= m_splash_z[spl] + ( m_splash_size * 0.5f );

		p_push[0]	= *((DWORD*)&vx0 );
		p_push[1]	= *((DWORD*)&vy );
		p_push[2]	= *((DWORD*)&vz0 );
		p_push[3]	= *((DWORD*)&icol );
		p_push[4]	= 0x00000000UL;
		p_push[5]	= 0x00000000UL;

		p_push[6]	= *((DWORD*)&vx1 );
		p_push[7]	= *((DWORD*)&vy );
		p_push[8]	= *((DWORD*)&vz0 );
		p_push[9]	= *((DWORD*)&icol );
		p_push[10]	= 0x3F800000UL;
		p_push[11]	= 0x00000000UL;

		p_push[12]	= *((DWORD*)&vx1 );
		p_push[13]	= *((DWORD*)&vy );
		p_push[14]	= *((DWORD*)&vz1 );
		p_push[15]	= *((DWORD*)&icol );
		p_push[16]	= 0x3F800000UL;
		p_push[17]	= 0x3F800000UL;

		p_push[18]	= *((DWORD*)&vx0 );
		p_push[19]	= *((DWORD*)&vy );
		p_push[20]	= *((DWORD*)&vz1 );
		p_push[21]	= *((DWORD*)&icol );
		p_push[22]	= 0x00000000UL;
		p_push[23]	= 0x3F800000UL;

		p_push			+= 24;
		dwords_written	+= 24;
		m_splash_current_life[spl]--;
	}

	p_push_encode_fixup[0]	= D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, dwords_written );
	p_push[0]				= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
	p_push[1]				= 0;
	D3DDevice_EndPush( p_push + 2 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxWeather::plat_render( void )
{
	if ( !m_system_active ) return;

	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = pSkate->GetSkater(0);
	if (!pSkater) return;
	
	float skx = pSkater->m_pos[X];
	float skz = pSkater->m_pos[Z];

	if ( m_raining )
	{
		plat_render_splashes( skx, skz );
		plat_render_rain( skx, skz );
	}
	else
	{
		plat_render_snow( skx, skz );
	}

	// Increment rain drop/snow flake positions.
	if ( m_active_drops )
	{
		for ( int lp = 0; lp < ( DROP_SIZE * DROP_SIZE * DROP_LAYERS ); lp++ )
		{
			switch ( m_drop_time[lp] )
			{
				case 255:
					// Inactive.
					break;
				case 254:
					// About to become inactive, so decrement active drops.
					m_active_drops--;
					// Deliberately falls through...
				default:
					// Increment time.
					m_drop_time[lp]++;
					break;
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxWeather::plat_set_rain_blend_mode( uint32 blendmode_checksum, int fix )
{
	m_rain_blend = NxXbox::GetBlendMode( blendmode_checksum ) | ((uint32)fix << 24 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxWeather::plat_set_splash_blend_mode( uint32 blendmode_checksum, int fix )
{
	m_splash_blend = NxXbox::GetBlendMode( blendmode_checksum ) | ((uint32)fix << 24 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxWeather::plat_set_snow_blend_mode( uint32 blendmode_checksum, int fix )
{
	m_snow_blend = NxXbox::GetBlendMode( blendmode_checksum ) | ((uint32)fix << 24 );
}

} // Nx




