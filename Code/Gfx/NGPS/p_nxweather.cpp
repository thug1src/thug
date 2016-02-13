//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxWeather.cpp
//* OWNER:          Paul Robinson
//* CREATION DATE:  6/2/2003
//****************************************************************************

#include <core/defines.h>
#include "gfx/ngps/nx/render.h"
#include "gfx/ngps/nx/dma.h"
#include "gfx/ngps/nx/vif.h"
#include "gfx/ngps/nx/vu1.h"
#include "gfx/ngps/nx/gif.h"
#include "gfx/ngps/nx/gs.h"

#include "gfx/ngps/nx/line.h"
#include <gfx/NxTexMan.h>
#include <gfx/ngps/p_nxtexture.h>

#include "gfx/ngps/nx/immediate.h"
#include "gfx/ngps/nx/vu1code.h"

#include "gfx/ngps/nx/mesh.h"

#include "gfx/ngps/p_nxweather.h"

#include <sk/modules/skate/skate.h>
#include <sk/objects/skater.h>

#include <sk/engine/feeler.h>
#include <engine/SuperSector.h>
#include "gfx/nx.h"

#include "gfx/ngps/nx/render.h"
#include "gfx/ngps/nx/dma.h"
#include "gfx/ngps/nx/vif.h"
#include "gfx/ngps/nx/vu1.h"
#include "gfx/ngps/nx/gif.h"
#include "gfx/ngps/nx/gs.h"
#include "gfx/ngps/nx/sprite.h"
#include "gfx/ngps/nx/switches.h"
#include "gfx/ngps/nx/vu1code.h"

#define FEELER_START 50000.0f

#define _STCYCL(a,b) ((uint32)0x01<<24 | (uint32)0<<16 | (uint32)((a)<<8|(b)))
#define _UNPACK(m,v,s,f,u,a) (((0x60|(m)<<4|(v))<<24) | ((s)<<16) | ((f)<<15 | (u)<<14 | ((NxPs2::vu1::Loc+(a))&0x03FF)))

#define _NREG(n) ((float)(n) * 1.1920928955e-07f)
#define _PRIM(n,f,p,r,a) (n)<<28 | (f)<<26 | (p)<<15 | (r)<<14 | (a)

#define _BEGINUNPACK(m,v,f,u,a,nbytes)																				\
					((uint32)(0x60 | (m)<<4 | (v))<<24 |														\
					(uint32)(( ((nbytes) << 3) / ( (((v)>>2 & 3) + 1) * (32 >> ((v) & 3)) ) ))<<16 |			\
					(uint32)((f)<<15 | (u)<<14 | ((NxPs2::vu1::Loc+(a))&0x03FF)))


#define _STMOD(o) ((uint32)0x05<<24 | (uint32)0<<16 | (uint32)(o)) 


#define _SIZE(v,nbytes) ((uint32)(( ((nbytes) << 3) / ( (((v)>>2 & 3) + 1) * (32 >> ((v) & 3)) ) )))

#define _PARSER ((uint32)0x14<<24 | (uint32)0<<16 | (uint32)VU1_ADDR(Parser)) 

#define NUM_LINES_PER_BATCH 80
#define NUM_SPRITES_PER_BATCH 80

#define PRECISION_SHIFT 4

unsigned char grid_bytes[70*1024];

#define RENDER_DIST 16
//#define RAIN_HEIGHT 2000
//#define RAIN_FRAMES 40
//#define RAIN_LENGTH 100.0f
#define SEQ_START 411		// Between 1 and 4095.
#define SEQ_MASK 0x0240
//#define SEQ_MASK 0x0ca0
//#define NUM_DROPS_PER_FRAME 25

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


//#define DEBUG_LINES

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2Weather::CPs2Weather()
{
	mp_roof_height_index = NULL;
	mp_rain_texture = NULL;

	m_rain_blend = NxPs2::CImmediateMode::sGetTextureBlend( CRCD(0xa86285a1,"fixadd"), 64 );		// FixAdd / 64
	m_splash_blend = NxPs2::CImmediateMode::sGetTextureBlend( CRCD(0xa86285a1,"fixadd"), 64 );		// FixAdd / 64
	m_snow_blend = NxPs2::CImmediateMode::sGetTextureBlend( CRCD(0xa86285a1,"fixadd"), 64 );		// FixAdd / 64

	m_rain_rate = 0.0f;
	m_splash_rate = 0.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2Weather::~CPs2Weather()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2Weather::plat_update_grid( void )
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
	if ( ( max_x - min_x ) > 350 )
	{
		int wid = ( max_x - min_x );
		int excess = ( wid - 350 );
		min_x += ( excess / 2 );
		max_x -= ( excess / 2 );
	}

	if ( ( max_z - min_z ) > 300 )
	{
		int wid = ( max_z - min_z );
		int excess = ( wid - 300 );
		min_z += ( excess / 2 );
		max_z -= ( excess / 2 );
	}

	// This is the actual width;
	m_width = ( max_x - min_x ) + 1;
	m_height = ( max_z - min_z ) + 1;

	// Allocate a new piece of memory for the grid.
//	if ( mp_roof_height_index ) delete mp_roof_height_index;
//	mp_roof_height_index = new unsigned char[m_width * m_height];

//	mp_roof_height_index = grid_bytes;

	m_min_x = ( min_x * WEATHER_CEL_SIZE );
	m_min_z = ( min_z * WEATHER_CEL_SIZE );

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

	printf( "Grid Size: Old: %d New: %d Num Heights: %d\n", ( m_width * m_height ), new_size, num_heights );
//	printf( "Num Weather Grid Heights: %d (%d x %d)\n", num_heights, m_width, m_height );

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
	Nx::CTexture *p_texture;
	Nx::CPs2Texture *p_ps2_texture;
	mp_rain_texture = NULL;

	// Set Rain Texture.
	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( CRCD(0x45c7eb0f,"splash") );
	p_ps2_texture = static_cast<Nx::CPs2Texture*>( p_texture );
	if ( p_ps2_texture )
	{
		mp_rain_texture = p_ps2_texture->GetSingleTexture(); 
	}

	// Set Snow Texture.
	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( CRCD(0xc97c5a4c,"snow") );
	p_ps2_texture = static_cast<Nx::CPs2Texture*>( p_texture );
	if ( p_ps2_texture )
	{
		mp_snow_texture = p_ps2_texture->GetSingleTexture(); 
	}

	m_system_active = true;

	// Zero out the splashes.
	for ( int sp = 0; sp < NUM_SPLASH_ACTIVE; sp++ )
	{
		m_splash_current_life[sp] = 0;
	}
	m_current_splash = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2Weather::plat_process( float delta_time )
{
	if ( !m_system_active ) return;

	if ( m_raining )
	{
		// It's raining.
		float rate = m_rain_drops_per_frame;
		if ( rate > (float)( ( DROP_SIZE * DROP_SIZE * DROP_LAYERS ) / m_rain_frames ) ) rate = (float)( ( DROP_SIZE * DROP_SIZE * DROP_LAYERS ) / m_rain_frames );

		float last = m_rain_rate;
		m_rain_rate += rate;
		int new_drops = (int)m_rain_rate - (int)last;

		for ( int lp = 0; lp < new_drops; lp++ )
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
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2Weather::plat_render_snow( float skx, float skz )
{
	// Early out if no drops to draw.
	if ( !m_active_drops ) return;

	// Get skater position.
	int x = (int)( ( skx - m_min_x ) / WEATHER_CEL_SIZE );
	int z = (int)( ( skz - m_min_z ) / WEATHER_CEL_SIZE );

	// Calculate area to render.
	int sx = x - RENDER_DIST;
	int ex = x + DROP_SIZE;	//RENDER_DIST;
	int sz = z - RENDER_DIST;
	int ez = z + DROP_SIZE;	//RENDER_DIST;

	// Clip z values.
	if ( ez < 0 ) return;
	if ( sz > ( m_height - 1 ) ) return;

//	if ( sz < 0 ) sz = 0;
//	if ( ez > ( m_height - 1 ) ) ez = ( m_height - 1 );

#ifdef DEBUG_LINES
	NxPs2::BeginLines3D( 0xffffffff );
#else
	// add a dma packet
	NxPs2::dma::BeginTag(NxPs2::dma::cnt, 0);

	// VU context
	NxPs2::vu1::BeginPrim(ABS, VU1_ADDR(L_VF10));
	NxPs2::vu1::StoreVec(*(NxPs2::Vec *)&NxPs2::render::InverseIntViewportScale);
	NxPs2::vu1::StoreVec(*(NxPs2::Vec *)&NxPs2::render::InverseIntViewportOffset);
	NxPs2::vu1::StoreMat(*(NxPs2::Mat *)&NxPs2::render::WorldToIntViewport);	// VF12-15
	NxPs2::vu1::EndPrim(0);
	NxPs2::vu1::BeginPrim(ABS, VU1_ADDR(L_VF30));
	NxPs2::vif::StoreV4_32F(640.0f, 480.0f, 0.0f, 0.0f);						// VF30
	NxPs2::vif::StoreV4_32F(NxPs2::render::IntViewportScale[0]/NxPs2::render::Tx,	// VF31
							NxPs2::render::IntViewportScale[1]/NxPs2::render::Ty,
							0.0f, 0.0f);
	NxPs2::vu1::EndPrim(0);

	// GS context
	NxPs2::gs::BeginPrim(ABS, 0, 0);
	NxPs2::gs::Reg1(NxPs2::gs::ALPHA_1,	m_snow_blend);
	NxPs2::gs::Reg1(NxPs2::gs::TEX0_1,	mp_snow_texture->m_RegTEX0);
	NxPs2::gs::Reg1(NxPs2::gs::TEX1_1,	mp_snow_texture->m_RegTEX1);
	NxPs2::gs::Reg1(NxPs2::gs::ST,		PackST(0x3F800000,0x3F800000));
	NxPs2::gs::Reg1(NxPs2::gs::RGBAQ,	PackRGBAQ(0,0,0,0,0x3F800000));
	NxPs2::gs::EndPrim(0);

#endif		// DEBUG_LINES

	float minx = m_min_x;
	float minz = m_min_z;
//	sint32 rlength = (sint32)( m_rain_length * SUB_INCH_PRECISION );
	
	// Calculate drop height list.
	int lp;
	float y_off[256];

	for ( lp = ( 254 - m_snow_frames ); lp < 256; lp++ )
	{
		y_off[lp] = m_snow_height - ( ( (float)( ( lp - 254 ) + m_snow_frames ) / (float)m_snow_frames ) * m_snow_height );
	}

	// Calculate xz offset list.
	float xz_off[256];

	for ( lp = 0; lp < 256; lp++ )
	{
		xz_off[lp] = ( (float)lp * (float)( WEATHER_CEL_SIZE / 2 ) ) / 255.0f;
	}

	// Calculate sine wave.
	float sin_off[256];

	for ( lp = 0; lp < 256; lp++ )
	{
		sin_off[lp] = sinf( Mth::PI * 2.0f * ( (float)lp / 256.0f ) ) * ( WEATHER_CEL_SIZE / 2 );
//		sin_off[lp] = sinf( Mth::PI * 2.0f * (float)lp ) * 128.0f;
	}

	int flakes = 0;
	uint32 * p_col = NULL;
	float * p_xyz = NULL;
//	uint32 * p_last_loc = NULL;
//	uint32 * p_tag = NULL;

	for ( int lzz = sz; lzz <= ez; lzz++ )
	{
		int zz = ( lzz < 0 ) ? 0 : ( lzz > ( m_height - 1 ) ) ? ( m_height - 1 ) : lzz;

		if ( mp_roof_row[zz].start == 16384 ) continue;

//		// Calculate actual span to scan.
//		int rsx = sx > mp_roof_row[zz].start ? sx : mp_roof_row[zz].start;
//		int rex = ex < mp_roof_row[zz].end ? ex : mp_roof_row[zz].end;
//
//		// Start position.
//		sint32 vx = ( (sint32)rsx * WEATHER_CEL_SIZE * (sint32)SUB_INCH_PRECISION ) + minx;
//		sint32 vz = ( (sint32)zz * WEATHER_CEL_SIZE * (sint32)SUB_INCH_PRECISION ) + minz;

		float vx = ( (float)sx * (float)WEATHER_CEL_SIZE ) + minx;
		float vz = ( (float)zz * (float)WEATHER_CEL_SIZE ) + minz;

		int cel = mp_roof_row[zz].index + ( sx - mp_roof_row[zz].start );

		int drop_cel_z = ( ( zz & ( DROP_SIZE - 1 ) ) << DROP_SIZE_SHIFT );

//		for ( int xx = rsx; xx <= rex; xx++, cel++ )
		for ( int lxx = sx; lxx <= ex; lxx++, cel++ )
		{
			int xx = ( lxx > mp_roof_row[zz].start ) ? ( ( lxx < mp_roof_row[zz].end ) ? lxx : mp_roof_row[zz].end ) : mp_roof_row[zz].start;

			// Get the current drop value. Skip this one if it's inactive.
			int drop_cel = drop_cel_z + ( xx & ( DROP_SIZE - 1 ) );

//			vx += ( WEATHER_CEL_SIZE << PRECISION_SHIFT );
			vx += (float)WEATHER_CEL_SIZE;
			float vy = (float)( m_roof_height[mp_roof_height_index[cel]] >> PRECISION_SHIFT );

			for ( int d = 0; d < DROP_LAYERS; d++, drop_cel += ( DROP_SIZE * DROP_SIZE ) )
			{
				if ( m_drop_time[drop_cel] == 255 ) continue;

				// Create the position for rendering.
				float v0x = vx + xz_off[m_x_offset[drop_cel]] + sin_off[(m_z_offset[drop_cel]+m_drop_time[drop_cel])&255];
				float v0y = vy + y_off[m_drop_time[drop_cel]] + m_snow_size;
				float v0z = vz + xz_off[m_z_offset[drop_cel]] + sin_off[(m_x_offset[drop_cel]+m_drop_time[drop_cel])&255];

//				sint32 v1y = v0y + rlength;

#ifdef DEBUG_LINES
				NxPs2::DrawLine3D( v0[X], v0[Y], v0[Z], v1[X], v1[Y], v1[Z] );
#else
				if ( flakes == 0 )
				{
					NxPs2::BeginModelPrimImmediate(NxPs2::gs::XYZ2		|
												   NxPs2::gs::ST<<4		|
												   NxPs2::gs::RGBAQ<<8	|
												   NxPs2::gs::XYZ2<<12,
												   4, SPRITE|ABE|TME, 1, VU1_ADDR(SpriteCull));

					// create an unpack for the colours
					NxPs2::vif::BeginUNPACK(0, V4_8, ABS, UNSIGNED, 3);
					p_col = (uint32 *)NxPs2::dma::pLoc;
					NxPs2::dma::pLoc += NUM_SPRITES_PER_BATCH * 4;
					NxPs2::vif::EndUNPACK();

					// and one for the positions (& sizes)
					NxPs2::vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 4);
					p_xyz = (float *)NxPs2::dma::pLoc;
					NxPs2::dma::pLoc += NUM_SPRITES_PER_BATCH * 16;
					NxPs2::vif::EndUNPACK();

					NxPs2::EndModelPrimImmediate(1);

					NxPs2::vif::MSCAL(VU1_ADDR(Parser));
				
					flakes = NUM_SPRITES_PER_BATCH;
				}

				*p_col++ = *((uint32 *)&m_snow_color);

				p_xyz[0] = v0x;
				p_xyz[1] = v0y;
				p_xyz[2] = v0z;
				p_xyz[3] = m_snow_size;
				p_xyz += 4;

				flakes--;

#endif		// DEBUG_LINES
			}
		}
	}
//	printf( "Drop comparison: Counted: %d Calculated: %d\n", drops, m_active_drops );

	// Fix the last packet.
//	if ( flakes == NUM_SPRITES_PER_BATCH )
//	{
//		NxPs2::vu1::Loc -= _SIZE(V4_32,16*NUM_SPRITES_PER_BATCH) * 2 + 1;
//		p_loc = p_last_loc;
//	}
//	else if ( lines )
//	{
	if ( flakes )
	{
		// Fill in with degenerate lines.
		for ( int l = 0; l < flakes; l++ )
		{
			p_xyz[0] = 0;
			p_xyz[1] = 0;
			p_xyz[2] = 0;
			p_xyz[3] = 0;

			p_xyz += 4;
		}
	}

#ifdef DEBUG_LINES
	NxPs2::EndLines3D();
#else
	// restore transform
	NxPs2::vu1::BeginPrim(ABS, VU1_ADDR(L_VF12));		// was L_VF16
	NxPs2::vu1::StoreMat(*(NxPs2::Mat *)&NxPs2::render::AdjustedWorldToViewport);		// VF16-19
	NxPs2::vu1::EndPrim(1);
	NxPs2::vif::MSCAL(VU1_ADDR(Parser));

	// end the dma tag
	NxPs2::dma::EndTag();
#endif		// DEBUG_LINES
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2Weather::plat_render_rain( float skx, float skz )
{
	// Early out if no drops to draw.
	if ( !m_active_drops ) return;

	// Get skater position.
	int x = (int)( ( skx - m_min_x ) / WEATHER_CEL_SIZE );
	int z = (int)( ( skz - m_min_z ) / WEATHER_CEL_SIZE );

	// Calculate area to render.
	int sx = x - RENDER_DIST;
	int ex = x + DROP_SIZE;	//RENDER_DIST;
	int sz = z - RENDER_DIST;
	int ez = z + DROP_SIZE;	//RENDER_DIST;

	// Clip z values.
	if ( ez < 0 ) return;
	if ( sz > ( m_height - 1 ) ) return;

//	if ( sz < 0 ) sz = 0;
//	if ( ez > ( m_height - 1 ) ) ez = ( m_height - 1 );

#ifdef DEBUG_LINES
	NxPs2::BeginLines3D( 0xffffffff );
#else
	NxPs2::BeginLines3D( 0xffffffff );
	NxPs2::EndLines3D();
	NxPs2::dma::BeginTag(NxPs2::dma::cnt, 0);
	NxPs2::CImmediateMode::sStartPolyDraw( NULL, m_rain_blend, ABS );
	register uint32 *p_loc = (uint32*)NxPs2::dma::pLoc;
#endif		// DEBUG_LINES

	sint32 minx = (sint32)( m_min_x * SUB_INCH_PRECISION );
	sint32 minz = (sint32)( m_min_z * SUB_INCH_PRECISION );
	sint32 rlength = (sint32)( m_rain_length * SUB_INCH_PRECISION );
	
	// Calculate drop height list.
	int lp;
	sint32 y_off[256];

	for ( lp = ( 254 - m_rain_frames ); lp < 256; lp++ )
	{
		y_off[lp] = (sint32)( ( m_rain_height - ( ( (float)( ( lp - 254 ) + m_rain_frames ) / (float)m_rain_frames ) * m_rain_height ) ) * SUB_INCH_PRECISION );
	}

	// Calculate xz offset list.
	sint32 xz_off[256];

	for ( lp = 0; lp < 256; lp++ )
	{
		xz_off[lp] = (sint32)( ( ( (float)lp * (float)WEATHER_CEL_SIZE * SUB_INCH_PRECISION ) / 255.0f ) );
	}

	int lines = 0;
	uint32 * p_col = NULL;
	sint32 * p_xyz = NULL;
	uint32 * p_last_loc = NULL;
//	uint32 * p_tag = NULL;

	for ( int lzz = sz; lzz <= ez; lzz++ )
	{
		int zz = ( lzz < 0 ) ? 0 : ( lzz > ( m_height - 1 ) ) ? ( m_height - 1 ) : lzz;

		if ( mp_roof_row[zz].start == 16384 ) continue;

//		// Calculate actual span to scan.
//		int rsx = sx > mp_roof_row[zz].start ? sx : mp_roof_row[zz].start;
//		int rex = ex < mp_roof_row[zz].end ? ex : mp_roof_row[zz].end;
//
//		// Start position.
//		sint32 vx = ( (sint32)rsx * WEATHER_CEL_SIZE * (sint32)SUB_INCH_PRECISION ) + minx;
//		sint32 vz = ( (sint32)zz * WEATHER_CEL_SIZE * (sint32)SUB_INCH_PRECISION ) + minz;

		sint32 vx = ( (sint32)sx << ( WEATHER_CEL_SIZE_SHIFT + PRECISION_SHIFT ) ) + minx;
		sint32 vz = ( (sint32)zz << ( WEATHER_CEL_SIZE_SHIFT + PRECISION_SHIFT ) ) + minz;

		int cel = mp_roof_row[zz].index + ( sx - mp_roof_row[zz].start );

		int drop_cel_z = ( ( zz & ( DROP_SIZE - 1 ) ) << DROP_SIZE_SHIFT );

//		for ( int xx = rsx; xx <= rex; xx++, cel++ )
		for ( int lxx = sx; lxx <= ex; lxx++, cel++ )
		{
			int xx = ( lxx > mp_roof_row[zz].start ) ? ( ( lxx < mp_roof_row[zz].end ) ? lxx : mp_roof_row[zz].end ) : mp_roof_row[zz].start;

			// Get the current drop value. Skip this one if it's inactive.
			int drop_cel = drop_cel_z + ( xx & ( DROP_SIZE - 1 ) );

			vx += ( WEATHER_CEL_SIZE << PRECISION_SHIFT );
			sint32 vy = m_roof_height[mp_roof_height_index[cel]];

			for ( int d = 0; d < DROP_LAYERS; d++, drop_cel += ( DROP_SIZE * DROP_SIZE ) )
			{
				if ( m_drop_time[drop_cel] == 255 ) continue;

				// Create the position for rendering.
				sint32 v0x = vx + xz_off[m_x_offset[drop_cel]];
				sint32 v0y = vy + y_off[m_drop_time[drop_cel]]; 
				sint32 v0z = vz + xz_off[m_z_offset[drop_cel]];

				sint32 v1y = v0y + rlength;

#ifdef DEBUG_LINES
				NxPs2::DrawLine3D( v0[X], v0[Y], v0[Z], v1[X], v1[Y], v1[Z] );
#else
				if ( lines == 0 )
				{
					p_last_loc = p_loc;

					p_loc[0] = _STCYCL(1,2);
					p_loc[1] = _UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
//					p_tag = &p_loc[2];
					*(float*)&p_loc[2] = _NREG(2);
					p_loc[2] |= (1<<15) | _SIZE(V4_32,16*2*NUM_LINES_PER_BATCH);

					p_loc[3] = _PRIM(2, PACKED, LINE|IIP|ABE, 1, VU1_ADDR(Line));
					p_loc[4] = NxPs2::gs::XYZ2<<4|NxPs2::gs::RGBAQ;
					p_loc[5] = ( _SIZE(V4_32,16*2*NUM_LINES_PER_BATCH) * 2 ); 
					p_loc[6] = _BEGINUNPACK(0, V4_8, ABS, UNSIGNED, 1, 4*2*NUM_LINES_PER_BATCH); 

					lines = NUM_LINES_PER_BATCH;

					p_col = &p_loc[7];
					p_loc = &p_loc[7+(2*NUM_LINES_PER_BATCH)];

					p_loc[0] = _STMOD(OFFSET_MODE); 
					p_loc[1] = _BEGINUNPACK(1, V4_32, ABS, SIGNED, 2, 16*2*NUM_LINES_PER_BATCH);

					p_xyz = (sint32 *)&p_loc[2];
					p_loc = &p_loc[2+(8*NUM_LINES_PER_BATCH)];
				
					p_loc[0] = _STMOD(NORMAL_MODE); 
	
					// finish batch of vertices
//					p_tag[0] |= (1<<15) | SIZE(V4_32,16*2*NUM_LINES_PER_BATCH);
					NxPs2::vu1::Loc += _SIZE(V4_32,16*2*NUM_LINES_PER_BATCH) * 2 + 1;
	
					p_loc[1] = _PARSER;
					NxPs2::vu1::Buffer = NxPs2::vu1::Loc;
	
					p_loc = &p_loc[2];

					// Fill color regardless.
					uint32 col0 = *((uint32 *)&m_rain_bottom_color); 
					uint32 col1 = *((uint32 *)&m_rain_top_color);    
//					for ( int lp = 0; lp < ( NUM_LINES_PER_BATCH / 4 ); lp++ )
//					{
//						p_col[0] = col0;
//						p_col[1] = col1;
//
//						p_col[2] = col0;
//						p_col[3] = col1;
//					
//						p_col[4] = col0;
//						p_col[5] = col1;
//					
//						p_col[6] = col0;
//						p_col[7] = col1;
//
//						p_col += 8;
//					}

					for ( int lp = 0; lp < NUM_LINES_PER_BATCH; lp++ )
					{
						p_col[0] = col0;
						p_col[1] = col1;

						p_col += 2;
					}
				}

//				p_col[0] = col0;
//				p_col[1] = col1;

				p_xyz[0] = v0x;
				p_xyz[1] = v0y;
				p_xyz[2] = v0z;
				p_xyz[3] = 0;
				p_xyz[4] = v0x;
				p_xyz[5] = v1y;
				p_xyz[6] = v0z;
				p_xyz[7] = 0;

//				p_col += 2;
				p_xyz += 8;

				lines --;
#endif		// DEBUG_LINES
			}
		}
	}
//	printf( "Drop comparison: Counted: %d Calculated: %d\n", drops, m_active_drops );

	// Fix the last packet.
	if ( lines == NUM_LINES_PER_BATCH )
	{
		NxPs2::vu1::Loc -= _SIZE(V4_32,16*2*NUM_LINES_PER_BATCH) * 2 + 1;
		p_loc = p_last_loc;
	}
	else if ( lines )
	{
		// Fill in with degenerate lines.
		for ( int l = 0; l < lines; l++ )
		{
			p_xyz[0] = 0;
			p_xyz[1] = 0;
			p_xyz[2] = 0;
			p_xyz[3] = 0;
			p_xyz[4] = 0;
			p_xyz[5] = 0;
			p_xyz[6] = 0;
			p_xyz[7] = 0;

			p_xyz += 8;
		}
	}

#ifdef DEBUG_LINES
	NxPs2::EndLines3D();
#else
	NxPs2::dma::pLoc = (uint8*)p_loc;
	NxPs2::dma::EndTag();





#endif		// DEBUG_LINES
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2Weather::plat_render_splashes( float skx, float skz )
{
	// Create a new splash if required.
	float last = m_splash_rate;
	m_splash_rate += m_splash_per_frame;
	int new_splashes = (int)m_splash_rate - (int)last;

	if ( new_splashes )
	{
		CFeeler feeler;
		Mth::Vector pos;

		pos[X] = NxPs2::render::CameraToWorld.GetPos()[X];	// + ( Mth::Rnd( ( WEATHER_CEL_SIZE * RENDER_DIST * 2 ) ) ) - ( WEATHER_CEL_SIZE * RENDER_DIST );
		pos[Y] = FEELER_START;
		pos[Z] = NxPs2::render::CameraToWorld.GetPos()[Z];	// + ( Mth::Rnd( ( WEATHER_CEL_SIZE * RENDER_DIST * 2 ) ) ) - ( WEATHER_CEL_SIZE * RENDER_DIST ); 
		pos[W] = 1.0f;

		Mth::Vector dir;
		dir[X] = skx - NxPs2::render::CameraToWorld.GetPos()[X];
		dir[Y] = 0.0f;
		dir[Z] = skz - NxPs2::render::CameraToWorld.GetPos()[Z];
		dir[W] = 1.0f;
		dir.Normalize();

		// Add distance.
		float r1 = (float)Mth::Rnd( 32768 ) / 32768.0f;
		pos += ( dir * ( (float)WEATHER_CEL_SIZE * RENDER_DIST * r1 ) );

		// Add lateral.
		Mth::Vector lat;
		lat[X] = dir[Z];
		lat[Y] = 0.0f;
		lat[Z] = -dir[X];
		lat[W] = 1.0f;

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

		m_splash_x[m_current_splash] = pos[X];
		m_splash_y[m_current_splash] = y + m_splash_size;
		m_splash_z[m_current_splash] = pos[Z];
		m_splash_current_life[m_current_splash] = m_splash_life;

		m_current_splash++;
		m_current_splash %= NUM_SPLASH_ACTIVE;
	}
	
	// Count active splashes.
	int splashes = 0;
	for ( int spl = 0; spl < NUM_SPLASH_ACTIVE; spl++ )
	{
		if ( m_splash_current_life[spl] != 0 ) splashes++;
	}

	if ( splashes )
	{
		// add a dma packet
		NxPs2::dma::BeginTag(NxPs2::dma::cnt, 0);

		// VU context
		NxPs2::vu1::BeginPrim(ABS, VU1_ADDR(L_VF10));
		NxPs2::vu1::StoreVec(*(NxPs2::Vec *)&NxPs2::render::InverseIntViewportScale);
		NxPs2::vu1::StoreVec(*(NxPs2::Vec *)&NxPs2::render::InverseIntViewportOffset);
		NxPs2::vu1::StoreMat(*(NxPs2::Mat *)&NxPs2::render::WorldToIntViewport);	// VF12-15
		NxPs2::vu1::EndPrim(0);
		NxPs2::vu1::BeginPrim(ABS, VU1_ADDR(L_VF30));
		NxPs2::vif::StoreV4_32F(640.0f, 480.0f, 0.0f, 0.0f);						// VF30
		NxPs2::vif::StoreV4_32F(NxPs2::render::IntViewportScale[0]/NxPs2::render::Tx,	// VF31
								NxPs2::render::IntViewportScale[1]/NxPs2::render::Ty,
								0.0f, 0.0f);
		NxPs2::vu1::EndPrim(0);

		// GS context
		NxPs2::gs::BeginPrim(ABS, 0, 0);
		NxPs2::gs::Reg1(NxPs2::gs::ALPHA_1,	m_splash_blend);
		NxPs2::gs::Reg1(NxPs2::gs::TEX0_1,	mp_rain_texture->m_RegTEX0);
		NxPs2::gs::Reg1(NxPs2::gs::TEX1_1,	mp_rain_texture->m_RegTEX1);
		NxPs2::gs::Reg1(NxPs2::gs::ST,		PackST(0x3F800000,0x3F800000));
		NxPs2::gs::Reg1(NxPs2::gs::RGBAQ,	PackRGBAQ(0,0,0,0,0x3F800000));
		NxPs2::gs::EndPrim(0);

		// Render the splashes.
	//	int u = mp_rain_texture->GetWidth() << 4;
	//	int v = mp_rain_texture->GetHeight() << 4;
	//	uint32 col = *((uint32 *)&m_splash_color); 

		NxPs2::BeginModelPrimImmediate(NxPs2::gs::XYZ2		|
									   NxPs2::gs::ST<<4		|
									   NxPs2::gs::RGBAQ<<8	|
									   NxPs2::gs::XYZ2<<12,
									   4, SPRITE|ABE|TME, 1, VU1_ADDR(SpriteCull));

		// create an unpack for the colours
		NxPs2::vif::BeginUNPACK(0, V4_8, ABS, UNSIGNED, 3);
		uint32 * p_col = (uint32 *)NxPs2::dma::pLoc;
		NxPs2::dma::pLoc += splashes * 4;
		NxPs2::vif::EndUNPACK();

		// and one for the positions (& sizes)
		NxPs2::vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 4);
		float * p_xyz = (float *)NxPs2::dma::pLoc;
		NxPs2::dma::pLoc += splashes * 16;
		NxPs2::vif::EndUNPACK();

		NxPs2::EndModelPrimImmediate(1);

		NxPs2::vif::MSCAL(VU1_ADDR(Parser));

		for ( int spl = 0; spl < NUM_SPLASH_ACTIVE; spl++ )
		{
			if ( m_splash_current_life[spl] == 0 ) continue;

			// Interpolate color.
			Image::RGBA	icol;
			icol.r = (uint8)( ( (int)m_splash_color.r * m_splash_current_life[spl] ) / m_splash_life );
			icol.g = (uint8)( ( (int)m_splash_color.g * m_splash_current_life[spl] ) / m_splash_life );
			icol.b = (uint8)( ( (int)m_splash_color.b * m_splash_current_life[spl] ) / m_splash_life );
			icol.a = (uint8)( ( (int)m_splash_color.a * m_splash_current_life[spl] ) / m_splash_life );

			// Draw splashes...
			*p_col++ = *((uint32 *)&icol);

			p_xyz[0] = m_splash_x[spl];
			p_xyz[1] = m_splash_y[spl];
			p_xyz[2] = m_splash_z[spl];
			p_xyz[3] = m_splash_size;
			p_xyz += 4;

			m_splash_current_life[spl]--;
		}

		// restore transform
		NxPs2::vu1::BeginPrim(ABS, VU1_ADDR(L_VF12));		// was L_VF16
		NxPs2::vu1::StoreMat(*(NxPs2::Mat *)&NxPs2::render::AdjustedWorldToViewport);		// VF16-19
		NxPs2::vu1::EndPrim(1);
		NxPs2::vif::MSCAL(VU1_ADDR(Parser));

		// end the dma tag
		NxPs2::dma::EndTag();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2Weather::plat_render( void )
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
void CPs2Weather::plat_set_rain_blend_mode( uint32 blendmode_checksum, int fix )
{
	m_rain_blend = NxPs2::CImmediateMode::sGetTextureBlend( blendmode_checksum, fix );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CPs2Weather::plat_set_splash_blend_mode( uint32 blendmode_checksum, int fix )
{
	m_splash_blend = NxPs2::CImmediateMode::sGetTextureBlend( blendmode_checksum, fix );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CPs2Weather::plat_set_snow_blend_mode( uint32 blendmode_checksum, int fix )
{
	m_snow_blend = NxPs2::CImmediateMode::sGetTextureBlend( blendmode_checksum, fix );
}

} // Nx




