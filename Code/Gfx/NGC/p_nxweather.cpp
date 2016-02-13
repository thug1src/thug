//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxWeather.cpp
//* OWNER:          Paul Robinson
//* CREATION DATE:  6/2/2003
//****************************************************************************

#include <core/defines.h>
#include "gfx/ngc/nx/render.h"
#include "gfx/ngc/nx/nx_init.h"

#include "gfx/ngc/nx/line.h"
#include <gfx/NxTexMan.h>
#include <gfx/ngc/p_nxtexture.h>

#include "gfx/ngc/nx/mesh.h"

#include "gfx/ngc/p_nxweather.h"
#include "gfx/ngc/p_nxparticle.h"

#include <sk/modules/skate/skate.h>
#include <sk/objects/skater.h>

#include <sk/engine/feeler.h>
#include <engine/SuperSector.h>
#include "gfx/nx.h"
#include <sys\ngc\p_gx.h>

#include "dolphin/base/ppcwgpipe.h"
#include "dolphin/gx/gxvert.h"

#define FEELER_START 50000.0f

unsigned char grid_bytes[45*1024];

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
	
CNgcWeather::CNgcWeather()
{
	mp_roof_height_index = NULL;
	mp_rain_texture = NULL;
	mp_snow_texture = NULL;

	m_rain_blend = get_texture_blend( CRCD(0xa86285a1,"fixadd") );		// FixAdd / 64
	m_splash_blend = get_texture_blend( CRCD(0xa86285a1,"fixadd") );		// FixAdd / 64
	m_snow_blend = get_texture_blend( CRCD(0xa86285a1,"fixadd") );		// FixAdd / 64

	m_rain_blend_fix = 64;
	m_splash_blend_fix = 64;
	m_snow_blend_fix = 64;

	m_rain_rate = 0.0f;
	m_splash_rate = 0.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CNgcWeather::~CNgcWeather()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CNgcWeather::plat_update_grid( void )
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
			float y;
			if ( feeler.GetCollision( false, false ) )		// No movables, nearest collision.
			{
				y = feeler.GetPoint()[Y];
			}
			else
			{
				y = FEELER_START;
			}

			// See if a close enough y pos already exists.
			int found = -1;
			for ( int lp = 0; lp < num_heights; lp++ )
			{
				if ( fabsf( ( m_roof_height[lp] - y ) ) < HEIGHT_TOLERANCE )
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

			if ( m_roof_height[p8[cel]] != FEELER_START )
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

#ifndef __NOPT_FINAL__
	int new_size = ( m_height * 6 ) + index;

	printf( "Grid Size: Old: %d New: %d Num Heights: %d\n", ( m_width * m_height ), new_size, num_heights );
#endif		// __NOPT_FINAL__
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

	Nx::CTexture *p_texture;
	Nx::CNgcTexture *p_Ngc_texture;

	// Set Rain Texture.
	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( CRCD(0x45c7eb0f,"splash") );
	p_Ngc_texture = static_cast<Nx::CNgcTexture*>( p_texture );
	if ( p_Ngc_texture )
	{
		mp_rain_texture = p_Ngc_texture->GetEngineTexture(); 
	}

	// Set Snow Texture.
	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( CRCD(0xc97c5a4c,"snow") );
	p_Ngc_texture = static_cast<Nx::CNgcTexture*>( p_texture );
	if ( p_Ngc_texture )
	{
		mp_snow_texture = p_Ngc_texture->GetEngineTexture(); 
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
	
void CNgcWeather::plat_process( float delta_time )
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
	
void CNgcWeather::plat_render_snow( float skx, float skz )
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

	GX::UploadTexture(  mp_snow_texture->pTexelData,
						mp_snow_texture->ActualWidth,
						mp_snow_texture->ActualHeight,
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
	GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, mp_snow_texture->ActualWidth, mp_snow_texture->ActualHeight );

	if ( mp_snow_texture->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA )
	{
		GX::UploadTexture(  mp_snow_texture->pAlphaData,
							mp_snow_texture->ActualWidth,
							mp_snow_texture->ActualHeight,
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
							GX_TEXMAP1 ); 
		GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, mp_snow_texture->ActualWidth, mp_snow_texture->ActualHeight );
		GX::SetTexChanTevIndCull( 2, 1, 2, 0, GX_CULL_NONE );
	}
	else
	{
		GX::SetTexChanTevIndCull( 1, 1, 1, 0, GX_CULL_NONE );
	}

	GX::SetTevOrder( GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP1, GX_COLOR0A0 );

	GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
										   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
										   GX_TEV_SWAP0, GX_TEV_SWAP0 );

	GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO, 
									   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );

	if ( mp_snow_texture->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA )
	{
		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO,
											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
											   GX_TEV_SWAP0, GX_TEV_SWAP1 );

		GX::SetTevColorInOp( GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV,
										   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );

	}

	switch ( m_snow_blend )
	{
		case NxNgc::vBLEND_MODE_ADD:
		case NxNgc::vBLEND_MODE_ADD_FIXED:
			GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_SUBTRACT:
		case NxNgc::vBLEND_MODE_SUB_FIXED:
			GX::SetBlendMode ( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_BLEND:
		case NxNgc::vBLEND_MODE_BLEND_FIXED:
			GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_BRIGHTEN:
		case NxNgc::vBLEND_MODE_BRIGHTEN_FIXED:
			GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_MODULATE_FIXED:
		case NxNgc::vBLEND_MODE_MODULATE:
			GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_BLEND_PREVIOUS_MASK:
		case NxNgc::vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK:
		case NxNgc::vBLEND_MODE_DIFFUSE:
		default:
			GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
	}

	GX::SetChanCtrl( GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
	GX::SetChanCtrl( GX_ALPHA0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 
	
	switch ( m_snow_blend )
	{
		case NxNgc::vBLEND_MODE_ADD_FIXED:
		case NxNgc::vBLEND_MODE_SUB_FIXED:
		case NxNgc::vBLEND_MODE_BLEND_FIXED:
		case NxNgc::vBLEND_MODE_BRIGHTEN_FIXED:
		case NxNgc::vBLEND_MODE_MODULATE_FIXED:
			{
				uint8 fix = m_snow_blend_fix >= 128 ? 255 : m_snow_blend_fix * 2;
				GX::SetChanMatColor( GX_COLOR0A0, (GXColor){fix,fix,fix,fix} );
			}
			break;
		default:
			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
			break;
	}
	
	GXColor col = (GXColor){ m_snow_color.r, m_snow_color.g, m_snow_color.b, m_snow_color.a };
	GX::SetChanAmbColor( GX_COLOR0A0, col );





	GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );

	float minx = m_min_x;
	float minz = m_min_z;
//	float rlength = m_rain_length;
	
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



	// Get the 'right' vector as the cross product of camera 'at and world 'up'.
	NsMatrix*	p_matrix	= &NxNgc::EngineGlobals.camera;

	NsVector	up( 0.0f, 1.0f, 0.0f );
	NsVector screen_right;
	NsVector screen_up;
	screen_right.cross( *p_matrix->getAt(), up );
	screen_up.cross( screen_right, *p_matrix->getAt());

	screen_right.normalize();
	screen_up.normalize();
	
	screen_right.x *= m_snow_size;
	screen_right.y *= m_snow_size;
	screen_right.z *= m_snow_size;

	screen_up.x *= m_snow_size;
	screen_up.y *= m_snow_size;
	screen_up.z *= m_snow_size;

	for ( int lzz = sz; lzz <= ez; lzz++ )
	{
		int zz = ( lzz < 0 ) ? 0 : ( lzz > ( m_height - 1 ) ) ? ( m_height - 1 ) : lzz;

		if ( mp_roof_row[zz].start == 16384 ) continue;

//		// Calculate actual span to scan.
//		int rsx = sx > mp_roof_row[zz].start ? sx : mp_roof_row[zz].start;
//		int rex = ex < mp_roof_row[zz].end ? ex : mp_roof_row[zz].end;
//
//		// Start position.
//		float vx = ( rsx * WEATHER_CEL_SIZE ) + minx;
//		float vz = ( zz * WEATHER_CEL_SIZE ) + minz;

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

			vx += (float)WEATHER_CEL_SIZE;
			float vy = m_roof_height[mp_roof_height_index[cel]];

			for ( int d = 0; d < DROP_LAYERS; d++, drop_cel += ( DROP_SIZE * DROP_SIZE ) )
			{
				if ( m_drop_time[drop_cel] == 255 ) continue;

				// Create the position for rendering.
				float v0x = vx + xz_off[m_x_offset[drop_cel]] + sin_off[(m_z_offset[drop_cel]+m_drop_time[drop_cel])&255];
				float v0y = vy + y_off[m_drop_time[drop_cel]] + m_snow_size;
				float v0z = vz + xz_off[m_z_offset[drop_cel]] + sin_off[(m_x_offset[drop_cel]+m_drop_time[drop_cel])&255];

				float v1x = v0x + screen_up.x;
				float v1y = v0y + screen_up.y;
				float v1z = v0z + screen_up.z;

				v0x -= screen_up.x;
				v0y -= screen_up.y;
				v0z -= screen_up.z;

				// Send coordinates.
				GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
					GXWGFifo.f32 = v1x - screen_right.x;
					GXWGFifo.f32 = v1y - screen_right.y;
					GXWGFifo.f32 = v1z - screen_right.z;
					GXWGFifo.f32 = 0.0f;
					GXWGFifo.f32 = 0.0f;

					GXWGFifo.f32 = v1x + screen_right.x;
					GXWGFifo.f32 = v1y + screen_right.y;
					GXWGFifo.f32 = v1z + screen_right.z;
					GXWGFifo.f32 = 1.0f;
					GXWGFifo.f32 = 0.0f;

					GXWGFifo.f32 = v0x + screen_right.x;
					GXWGFifo.f32 = v0y + screen_right.y;
					GXWGFifo.f32 = v0z + screen_right.z;
					GXWGFifo.f32 = 1.0f;
					GXWGFifo.f32 = 1.0f;

					GXWGFifo.f32 = v0x - screen_right.x;
					GXWGFifo.f32 = v0y - screen_right.y;
					GXWGFifo.f32 = v0z - screen_right.z;
					GXWGFifo.f32 = 0.0f;
					GXWGFifo.f32 = 1.0f;
//					GX::Position3f32( v1x - screen_right.x, v1y - screen_right.y, v1z - screen_right.z );
//					GX::Position3f32( v1x + screen_right.x, v1y + screen_right.y, v1z + screen_right.z );
//					GX::Position3f32( v0x + screen_right.x, v0y + screen_right.y, v0z + screen_right.z );
//					GX::Position3f32( v0x - screen_right.x, v0y - screen_right.y, v0z - screen_right.z );
				GX::End();
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CNgcWeather::plat_render_rain( float skx, float skz )
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

	GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);

	GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
										   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_ENABLE, GX_TEVPREV,
										   GX_TEV_SWAP0, GX_TEV_SWAP0 );

	GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC,
									   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_ENABLE, GX_TEVPREV );

	// Set blend mode for base layer.
	switch ( m_rain_blend )
	{
		case NxNgc::vBLEND_MODE_ADD:
		case NxNgc::vBLEND_MODE_ADD_FIXED:
			GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_SUBTRACT:
		case NxNgc::vBLEND_MODE_SUB_FIXED:
			GX::SetBlendMode ( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_BLEND:
		case NxNgc::vBLEND_MODE_BLEND_FIXED:
			GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_BRIGHTEN:
		case NxNgc::vBLEND_MODE_BRIGHTEN_FIXED:
			GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_MODULATE_FIXED:
		case NxNgc::vBLEND_MODE_MODULATE:
			GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
		case NxNgc::vBLEND_MODE_BLEND_PREVIOUS_MASK:
		case NxNgc::vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK:
		case NxNgc::vBLEND_MODE_DIFFUSE:
		default:
			GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
			break;
	}

	GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
	
	switch ( m_rain_blend )
	{
		case NxNgc::vBLEND_MODE_ADD_FIXED:
		case NxNgc::vBLEND_MODE_SUB_FIXED:
		case NxNgc::vBLEND_MODE_BLEND_FIXED:
		case NxNgc::vBLEND_MODE_BRIGHTEN_FIXED:
		case NxNgc::vBLEND_MODE_MODULATE_FIXED:
			{
				uint8 fix = m_rain_blend_fix >= 128 ? 255 : m_rain_blend_fix * 2;
				GX::SetChanMatColor( GX_COLOR0A0, (GXColor){fix,fix,fix,fix} );
			}
			break;
		default:
			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
			break;
	}

	GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT );

	float minx = m_min_x;
	float minz = m_min_z;
	float rlength = m_rain_length;
	
	// Calculate drop height list.
	int lp;
	float y_off[256];

	for ( lp = ( 254 - m_rain_frames ); lp < 256; lp++ )
	{
		y_off[lp] = ( m_rain_height - ( (float)( ( lp - 254 ) + m_rain_frames ) / (float)m_rain_frames ) * m_rain_height );
	}

	// Calculate xz offset list.
	float xz_off[256];

	for ( lp = 0; lp < 256; lp++ )
	{
		xz_off[lp] = ( (float)lp * (float)WEATHER_CEL_SIZE ) / 255.0f;
	}

	GXColor colb = (GXColor){ m_rain_bottom_color.r, m_rain_bottom_color.g, m_rain_bottom_color.b, m_rain_bottom_color.a };
	GXColor colt = (GXColor){ m_rain_top_color.r, m_rain_top_color.g, m_rain_top_color.b, m_rain_top_color.a };

	for ( int lzz = sz; lzz <= ez; lzz++ )
	{
		int zz = ( lzz < 0 ) ? 0 : ( lzz > ( m_height - 1 ) ) ? ( m_height - 1 ) : lzz;

		if ( mp_roof_row[zz].start == 16384 ) continue;

//		// Calculate actual span to scan.
//		int rsx = sx > mp_roof_row[zz].start ? sx : mp_roof_row[zz].start;
//		int rex = ex < mp_roof_row[zz].end ? ex : mp_roof_row[zz].end;
//
//		// Start position.
//		float vx = ( rsx * WEATHER_CEL_SIZE ) + minx;
//		float vz = ( zz * WEATHER_CEL_SIZE ) + minz;

		float vx = (float)( sx << WEATHER_CEL_SIZE_SHIFT ) + minx;
		float vz = (float)( zz << WEATHER_CEL_SIZE_SHIFT ) + minz;

		int cel = mp_roof_row[zz].index + ( sx - mp_roof_row[zz].start );

		int drop_cel_z = ( ( zz & ( DROP_SIZE - 1 ) ) << DROP_SIZE_SHIFT );

//		for ( int xx = rsx; xx <= rex; xx++, cel++ )
		for ( int lxx = sx; lxx <= ex; lxx++, cel++ )
		{
			int xx = ( lxx > mp_roof_row[zz].start ) ? ( ( lxx < mp_roof_row[zz].end ) ? lxx : mp_roof_row[zz].end ) : mp_roof_row[zz].start;

			// Get the current drop value. Skip this one if it's inactive.
			int drop_cel = drop_cel_z + ( xx & ( DROP_SIZE - 1 ) );

			vx += (float)WEATHER_CEL_SIZE;
			float vy = m_roof_height[mp_roof_height_index[cel]];

			for ( int d = 0; d < DROP_LAYERS; d++, drop_cel += ( DROP_SIZE * DROP_SIZE ) )
			{
				if ( m_drop_time[drop_cel] == 255 ) continue;

				// Create the position for rendering.
				float v0x = vx + xz_off[m_x_offset[drop_cel]];
				float v0y = vy + y_off[m_drop_time[drop_cel]]; 
				float v0z = vz + xz_off[m_z_offset[drop_cel]];

				float v1y = v0y + rlength;

				// Send coordinates.
				GX::Begin( GX_LINES, GX_VTXFMT0, 2 ); 
					GX::Position3f32( v0x, v0y, v0z );
					GX::Color1u32( *((uint32*)&colb) );
					GX::Position3f32( v0x, v1y, v0z );
					GX::Color1u32( *((uint32*)&colt) );
				GX::End();
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CNgcWeather::plat_render_splashes( float skx, float skz )
{
	// Create a new splash if required.
	float last = m_splash_rate;
	m_splash_rate += m_splash_per_frame;
	int new_splashes = (int)m_splash_rate - (int)last;

	if ( new_splashes )
	{
		CFeeler feeler;
		Mth::Vector pos;

		pos[X] = NxNgc::EngineGlobals.camera.getPosX();	// + ( Mth::Rnd( ( WEATHER_CEL_SIZE * RENDER_DIST * 2 ) ) ) - ( WEATHER_CEL_SIZE * RENDER_DIST );
		pos[Y] = FEELER_START;
		pos[Z] = NxNgc::EngineGlobals.camera.getPosZ();	// + ( Mth::Rnd( ( WEATHER_CEL_SIZE * RENDER_DIST * 2 ) ) ) - ( WEATHER_CEL_SIZE * RENDER_DIST ); 
		pos[W] = 1.0f;

		Mth::Vector dir;
		dir[X] = skx - NxNgc::EngineGlobals.camera.getPosX();
		dir[Y] = 0.0f;
		dir[Z] = skz - NxNgc::EngineGlobals.camera.getPosZ();
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
		GX::UploadTexture(  mp_rain_texture->pTexelData,
							mp_rain_texture->ActualWidth,
							mp_rain_texture->ActualHeight,
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
		GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, mp_rain_texture->ActualWidth, mp_rain_texture->ActualHeight );

		if ( mp_rain_texture->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA )
		{
			GX::UploadTexture(  mp_rain_texture->pAlphaData,
								mp_rain_texture->ActualWidth,
								mp_rain_texture->ActualHeight,
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
								GX_TEXMAP1 ); 
			GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, mp_rain_texture->ActualWidth, mp_rain_texture->ActualHeight );
			GX::SetTexChanTevIndCull( 2, 1, 2, 0, GX_CULL_NONE );
		}
		else
		{
			GX::SetTexChanTevIndCull( 1, 1, 1, 0, GX_CULL_NONE );
		}

		GX::SetTevOrder( GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP1, GX_COLOR0A0 );

		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
											   GX_TEV_SWAP0, GX_TEV_SWAP0 );

		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO, 
										   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );

		if ( mp_rain_texture->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA )
		{
			GX::SetTevAlphaInOpSwap( GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO,
												   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
												   GX_TEV_SWAP0, GX_TEV_SWAP1 );

			GX::SetTevColorInOp( GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV,
											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );

		}

		switch ( m_splash_blend )
		{
			case NxNgc::vBLEND_MODE_ADD:
			case NxNgc::vBLEND_MODE_ADD_FIXED:
				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case NxNgc::vBLEND_MODE_SUBTRACT:
			case NxNgc::vBLEND_MODE_SUB_FIXED:
				GX::SetBlendMode ( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case NxNgc::vBLEND_MODE_BLEND:
			case NxNgc::vBLEND_MODE_BLEND_FIXED:
				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case NxNgc::vBLEND_MODE_BRIGHTEN:
			case NxNgc::vBLEND_MODE_BRIGHTEN_FIXED:
				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case NxNgc::vBLEND_MODE_MODULATE_FIXED:
			case NxNgc::vBLEND_MODE_MODULATE:
				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case NxNgc::vBLEND_MODE_BLEND_PREVIOUS_MASK:
			case NxNgc::vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK:
			case NxNgc::vBLEND_MODE_DIFFUSE:
			default:
				GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
		}

//		GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
		GX::SetChanCtrl( GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
		GX::SetChanCtrl( GX_ALPHA0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 

		switch ( m_splash_blend )
		{
			case NxNgc::vBLEND_MODE_ADD_FIXED:
			case NxNgc::vBLEND_MODE_SUB_FIXED:
			case NxNgc::vBLEND_MODE_BLEND_FIXED:
			case NxNgc::vBLEND_MODE_BRIGHTEN_FIXED:
			case NxNgc::vBLEND_MODE_MODULATE_FIXED:
				{
					uint8 fix = m_splash_blend_fix >= 128 ? 255 : m_splash_blend_fix * 2;
					GX::SetChanMatColor( GX_COLOR0A0, (GXColor){fix,fix,fix,fix} );
				}
				break;
			default:
				GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
				break;
		}

		GXColor col = (GXColor){ m_splash_color.r, m_splash_color.g, m_splash_color.b, m_splash_color.a };
		GX::SetChanAmbColor( GX_COLOR0A0, col );





		GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );

		// Get the 'right' vector as the cross product of camera 'at and world 'up'.
		NsMatrix*	p_matrix	= &NxNgc::EngineGlobals.camera;

		NsVector	up( 0.0f, 1.0f, 0.0f );
		NsVector screen_right;
		NsVector screen_up;
		screen_right.cross( *p_matrix->getAt(), up );
		screen_up.cross( screen_right, *p_matrix->getAt());

		screen_right.normalize();
		screen_up.normalize();

		screen_right.x *= m_splash_size;
		screen_right.y *= m_splash_size;
		screen_right.z *= m_splash_size;

		screen_up.x *= m_splash_size;
		screen_up.y *= m_splash_size;
		screen_up.z *= m_splash_size;

		// Render the splashes.
	//	int u = mp_rain_texture->GetWidth() << 4;
	//	int v = mp_rain_texture->GetHeight() << 4;
	//	uint32 col = *((uint32 *)&m_splash_color); 

		for ( int spl = 0; spl < NUM_SPLASH_ACTIVE; spl++ )
		{
			if ( m_splash_current_life[spl] == 0 ) continue;

			// Interpolate color.
			GXColor	icol;
			icol.r = (uint8)( ( (int)m_splash_color.r * m_splash_current_life[spl] ) / m_splash_life );
			icol.g = (uint8)( ( (int)m_splash_color.g * m_splash_current_life[spl] ) / m_splash_life );
			icol.b = (uint8)( ( (int)m_splash_color.b * m_splash_current_life[spl] ) / m_splash_life );
			icol.a = (uint8)( ( (int)m_splash_color.a * m_splash_current_life[spl] ) / m_splash_life );

			float v0x = m_splash_x[spl] - screen_up.x;
			float v0y = m_splash_y[spl] - screen_up.y;
			float v0z = m_splash_z[spl] - screen_up.z;

			float v1x = m_splash_x[spl] + screen_up.x; 
			float v1y = m_splash_y[spl] + screen_up.y; 
			float v1z = m_splash_z[spl] + screen_up.z; 

			// Draw splashes...
			GX::SetChanMatColor( GX_COLOR0A0, icol );
			GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
				GXWGFifo.f32 = v1x - screen_right.x;
				GXWGFifo.f32 = v1y - screen_right.y;
				GXWGFifo.f32 = v1z - screen_right.z;
				GXWGFifo.f32 = 0.0f;
				GXWGFifo.f32 = 0.0f;
	
				GXWGFifo.f32 = v1x + screen_right.x;
				GXWGFifo.f32 = v1y + screen_right.y;
				GXWGFifo.f32 = v1z + screen_right.z;
				GXWGFifo.f32 = 1.0f;
				GXWGFifo.f32 = 0.0f;
	
				GXWGFifo.f32 = v0x + screen_right.x;
				GXWGFifo.f32 = v0y + screen_right.y;
				GXWGFifo.f32 = v0z + screen_right.z;
				GXWGFifo.f32 = 1.0f;
				GXWGFifo.f32 = 1.0f;
	
				GXWGFifo.f32 = v0x - screen_right.x;
				GXWGFifo.f32 = v0y - screen_right.y;
				GXWGFifo.f32 = v0z - screen_right.z;
				GXWGFifo.f32 = 0.0f;
				GXWGFifo.f32 = 1.0f;
			GX::End();

			m_splash_current_life[spl]--;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CNgcWeather::plat_render( void )
{
	if ( !m_system_active ) return;

	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = pSkate->GetSkater(0);

	float skx = pSkater->m_pos[X];
	float skz = pSkater->m_pos[Z];

	GX::LoadPosMtxImm( (MtxPtr)&NxNgc::EngineGlobals.world_to_camera, GX_PNMTX0 );
	GX::SetZMode( GX_TRUE, GX_LEQUAL, GX_FALSE );

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
void CNgcWeather::plat_set_rain_blend_mode( uint32 blendmode_checksum, int fix )
{
	m_rain_blend = get_texture_blend( blendmode_checksum );
	m_rain_blend_fix = fix;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcWeather::plat_set_splash_blend_mode( uint32 blendmode_checksum, int fix )
{
	m_splash_blend = get_texture_blend( blendmode_checksum );
	m_splash_blend_fix = fix;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcWeather::plat_set_snow_blend_mode( uint32 blendmode_checksum, int fix )
{
	m_snow_blend = get_texture_blend( blendmode_checksum );
	m_snow_blend_fix = fix;
}

} // Nx





