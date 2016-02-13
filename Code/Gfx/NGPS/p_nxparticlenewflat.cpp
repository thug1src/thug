//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticle.cpp
//* OWNER:          Paul Robinson
//* CREATION DATE:  3/27/2002
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
#include "gfx/ngps/nx/group.h"

#include "gfx/ngps/p_nxparticlenewflat.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleNewFlat::CPs2ParticleNewFlat()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleNewFlat::CPs2ParticleNewFlat( uint32 checksum, int max_streams, uint32 texture_checksum, uint32 blendmode_checksum, int fix )
{
	m_checksum = checksum;
	m_max_streams = max_streams;
	m_num_streams = 0;

	// Get the texture.
	Nx::CTexture *p_texture;
	Nx::CPs2Texture *p_ps2_texture;
	mp_engine_texture = NULL;

	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( texture_checksum );
	p_ps2_texture = static_cast<Nx::CPs2Texture*>( p_texture );
	if ( p_ps2_texture )
	{
		mp_engine_texture = p_ps2_texture->GetSingleTexture(); 
	}

	// Set blendmode.
	m_blend = NxPs2::CImmediateMode::sGetTextureBlend( blendmode_checksum, fix );

	// Allocate space for span information & streams. 
	m_num_spans = 1;		// Currently defaulting to 1 span.
	mp_span = new CParticleSpan[m_num_spans];
	mp_stream = new CParticleStream[m_max_streams]; 

	mp_newest_stream = mp_stream + m_max_streams - 1;
	mp_oldest_stream = mp_stream;
	m_life = 0.5f * (m_life_min + m_life_max);
	m_emitting = false;


	// set up system dma data, apart from new coefficients, which are calculated in plat_build_path()
	m_systemDmaData.m_GScontext[0] = 0x00008004;
	m_systemDmaData.m_GScontext[1] = 0x10000000;
	m_systemDmaData.m_GScontext[2] = 0x0000000E;
	m_systemDmaData.m_GScontext[3] = 0x00000000;

	*(uint64 *)&m_systemDmaData.m_GScontext[4] = m_blend;
	m_systemDmaData.m_GScontext[6] = 0x00000042;

	if (mp_engine_texture)
	{
		m_systemDmaData.m_u0 = 8;
		m_systemDmaData.m_v0 = (mp_engine_texture->GetHeight()<<4) - 8;
		m_systemDmaData.m_u1 = (mp_engine_texture->GetWidth() <<4) - 8;
		m_systemDmaData.m_v1 = 8;
		//m_systemDmaData.m_GScontext[16] = ((mp_engine_texture->GetHeight()<<4) - 8) << 16 | 8;

		*(uint64 *)&m_systemDmaData.m_GScontext[8] = mp_engine_texture->m_RegTEX0;
		*(uint64 *)&m_systemDmaData.m_GScontext[12] = mp_engine_texture->m_RegTEX1;
	}
	m_systemDmaData.m_GScontext[10] = 0x00000006;
	m_systemDmaData.m_GScontext[14] = 0x00000014;

	m_systemDmaData.m_GScontext[16] = 0x0005000B /* | m_alphaCutoff<<4 */;	//101 0000 xxxx xxxx 1011
	m_systemDmaData.m_GScontext[18] = 0x00000047;

	m_systemDmaData.m_tagy = 0x60AB4000;
	m_systemDmaData.m_tagz = 0x00434312;


	m_mid_time = -1.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleNewFlat::~CPs2ParticleNewFlat()
{
	delete [] mp_span;
	delete [] mp_stream;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleNewFlat::plat_get_position( int entry, int list, float * x, float * y, float * z )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleNewFlat::plat_set_position( int entry, int list, float x, float y, float z )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleNewFlat::plat_add_position( int entry, int list, float x, float y, float z )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CPs2ParticleNewFlat::plat_get_num_particle_colors( void ) { return 1; };
int CPs2ParticleNewFlat::plat_get_num_vertex_lists( void ) { return 0; };
void CPs2ParticleNewFlat::plat_set_sr( int entry, uint8 value ) { m_start_color.r = value; }
void CPs2ParticleNewFlat::plat_set_sg( int entry, uint8 value ) { m_start_color.g = value; }
void CPs2ParticleNewFlat::plat_set_sb( int entry, uint8 value ) { m_start_color.b = value; }
void CPs2ParticleNewFlat::plat_set_sa( int entry, uint8 value ) { m_start_color.a = value >> 1; }
void CPs2ParticleNewFlat::plat_set_mr( int entry, uint8 value ) { m_mid_color.r = value; }
void CPs2ParticleNewFlat::plat_set_mg( int entry, uint8 value ) { m_mid_color.g = value; }
void CPs2ParticleNewFlat::plat_set_mb( int entry, uint8 value ) { m_mid_color.b = value; }
void CPs2ParticleNewFlat::plat_set_ma( int entry, uint8 value ) { m_mid_color.a = value >> 1; }
void CPs2ParticleNewFlat::plat_set_er( int entry, uint8 value ) { m_end_color.r = value; }
void CPs2ParticleNewFlat::plat_set_eg( int entry, uint8 value ) { m_end_color.g = value; }
void CPs2ParticleNewFlat::plat_set_eb( int entry, uint8 value ) { m_end_color.b = value; }
void CPs2ParticleNewFlat::plat_set_ea( int entry, uint8 value ) { m_end_color.a = value >> 1; }


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	

void CPs2ParticleNewFlat::plat_render( void )
{
	CParticleStream *p_stream;
	int i;


	//--------------------------------------------------------------------------------------------------------------------
	// process the streams
	if (m_emit_rate && (!m_emitting || (m_emit_rate != mp_newest_stream->m_rate)))
	{	
		if (m_num_streams < m_max_streams)
		{
			// add new stream to cyclic buffer
			m_num_streams++;
			mp_newest_stream++;
			if (mp_newest_stream == mp_stream + m_max_streams)
			{
				mp_newest_stream = mp_stream;
			}

			// initialise new stream
			mp_newest_stream->m_rate			= m_emit_rate;
			mp_newest_stream->m_interval		= 1.0f/m_emit_rate;
			mp_newest_stream->m_oldest_age		= 0.0f;
			mp_newest_stream->m_num_particles	= 0;
			mp_newest_stream->m_seed[0]			= rand();
			mp_newest_stream->m_seed[1]			= rand();
			mp_newest_stream->m_seed[2]			= rand();
			mp_newest_stream->m_seed[3]			= rand();
			m_emitting = true;
		}
		else
		{
			m_emitting = false;
		}
	}
	else
	{
		m_emitting = m_emit_rate;
	}

	if (!m_num_streams)
		return;

	// age all streams
	for (i=0, p_stream=mp_oldest_stream; i<m_num_streams; i++)
	{
		// increase age of oldest particle
		p_stream->m_oldest_age += 1.0f/60.0f;

		// step pointer within cyclic buffer
		p_stream++;
		if (p_stream == mp_stream + m_max_streams)
		{
			p_stream = mp_stream;
		}
	}

	// births into newest stream
	if (m_emitting)
	{
		// how many particles so far emitted?
		mp_newest_stream->m_num_particles = (int)(mp_newest_stream->m_oldest_age * mp_newest_stream->m_rate + 1.0f);
	}

	// deaths from oldest stream
	if (mp_oldest_stream->m_oldest_age > m_life)
	{
		// work out number dead
		int particles_dead = (int)((mp_oldest_stream->m_oldest_age - m_life) * mp_oldest_stream->m_rate + 1.0f);

		// remove dead particles
		mp_oldest_stream->m_num_particles -= particles_dead;

		// should we keep processing the oldest stream?
		if (mp_oldest_stream->m_num_particles>0 || (m_num_streams==1 && m_emitting))
		{
			// adjust age of oldest particle
			mp_oldest_stream->m_oldest_age -= (float)particles_dead * mp_oldest_stream->m_interval;

			// advance seed
			mp_oldest_stream->AdvanceSeed(particles_dead);
		}
		else
		{
			// remove oldest stream and wrap in cyclic buffer if necessary
			m_num_streams--;
			mp_oldest_stream++;
			if (mp_oldest_stream == mp_stream + m_max_streams)
			{
				mp_oldest_stream = mp_stream;
			}
			if (!m_num_streams)
				return;
		}
	}


	//--------------------------------------------------------------------------------------------------------------------
	// now render the streams

	if (mp_engine_texture)
	{
		// Mick:  If this is textured, we reset the TEX0, TEX1 regs as the texture might move
		// Since 2D textures are dynamically packed every frame
		*(uint64 *)&m_systemDmaData.m_GScontext[8] = mp_engine_texture->m_RegTEX0;
		*(uint64 *)&m_systemDmaData.m_GScontext[12] = mp_engine_texture->m_RegTEX1;
	}

	// matrix is system rotation times view transform
	m_systemDmaData.m_matrix = m_rotation * NxPs2::render::WorldToIntViewport;

	// in the per-frame setup dma we must have:
	// dma tag
	// base
	// offset

	// set the group
	NxPs2::dma::SetList(NxPs2::sGroup::pParticles);

	// the system and streams will be loaded to a double-buffered input area of VUMem1
	// ref the system data, and include an unpack for the system and streams:
	NxPs2::dma::Tag(NxPs2::dma::ref, (sizeof(CSystemDmaDataFlat)>>4), (uint)&m_systemDmaData);
	NxPs2::vif::STCYCL(1,1);
	NxPs2::vif::UNPACK(0, V4_32, (sizeof(CSystemDmaDataFlat)>>4)+m_num_streams*2, REL, SIGNED, 0);

	// construct a packet with data for each stream
	NxPs2::dma::BeginTag(NxPs2::dma::cnt, 0);
	for (i=0,p_stream=mp_oldest_stream; i<m_num_streams; i++,p_stream++)
	{
		Dbg_MsgAssert(m_num_particles<65536, ("particle limit reached (65536)"));

		// wrap at end of cyclic buffer
		if (p_stream == mp_stream + m_max_streams)
		{
			p_stream = mp_stream;
		}

		((float  *)NxPs2::dma::pLoc)[0] = p_stream->m_oldest_age;
		((float  *)NxPs2::dma::pLoc)[1] = p_stream->m_interval;
		((uint32 *)NxPs2::dma::pLoc)[2] = p_stream->m_num_particles;
		((uint32 *)NxPs2::dma::pLoc)[3] = (p_stream==mp_newest_stream) ? 0x8000 : 0;
		((uint32 *)NxPs2::dma::pLoc)[4] = p_stream->m_seed[0];
		((uint32 *)NxPs2::dma::pLoc)[5] = p_stream->m_seed[1];
		((uint32 *)NxPs2::dma::pLoc)[6] = p_stream->m_seed[2];
		((uint32 *)NxPs2::dma::pLoc)[7] = p_stream->m_seed[3];

		//printf("stream %d, num particles=%d, oldest age=%f\n", s, p_stream->m_num_particles, p_stream->m_oldest_age);

		// step dma pointer
		NxPs2::dma::pLoc += 8*4;

		// count particles
		NxPs2::render::sTotalNewParticles += p_stream->m_num_particles;
	}

	NxPs2::vif::MSCAL(10);	// sprites
	NxPs2::dma::EndTag();
	NxPs2::dma::SetList(NxPs2::sGroup::pEpilogue);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleNewFlat::plat_build_path( void )
{
	float midTime;

	// set m_life
	m_life = 0.5f * (m_life_min + m_life_max);

	// set clamped local midtime variable
	if (m_mid_time <= 0.0f)
	{
		midTime = 0.0f;
	}
	else if (m_mid_time >= 100.0f)
	{
		midTime = m_life;
	}
	else
	{
		midTime = m_mid_time * 0.01f * m_life;
	}

	// x-component will be overwritten by vu1 code, so can store midtime there
	*(float *)&m_systemDmaData.m_tagx = midTime;


	// constant part
	m_systemDmaData.m_s0x = m_emit_w;
	m_systemDmaData.m_s0y = 0.0f;
	m_systemDmaData.m_s0z = m_emit_h;
	m_systemDmaData.m_s0w = 0.0f;

	m_systemDmaData.m_c0x = m_pos[0] - 1.5f * m_systemDmaData.m_s0x;
	m_systemDmaData.m_c0y = m_pos[1] - 1.5f * m_systemDmaData.m_s0y;
	m_systemDmaData.m_c0z = m_pos[2] - 1.5f * m_systemDmaData.m_s0z;
	m_systemDmaData.m_c0w = m_sw     - 1.5f * m_systemDmaData.m_s0w;


	// linear part
	Mth::Vector vel(m_tx,m_ty,m_tz,0.0f);
	float speed = 60.0f * 0.5 * (m_speed_min + m_speed_max);
	vel.Normalize();
	vel *= speed;
	m_systemDmaData.m_s1x = speed * Mth::PI/2.0f * m_emit_angle_spread;
	m_systemDmaData.m_s1y = 60.0f * (m_speed_max - m_speed_min);
	m_systemDmaData.m_s1z = speed * Mth::PI/2.0f * m_emit_angle_spread;
	m_systemDmaData.m_s1w = 0.0f;

	m_systemDmaData.m_c1x = vel[0]               - 1.5f * m_systemDmaData.m_s1x;
	m_systemDmaData.m_c1y = vel[1]               - 1.5f * m_systemDmaData.m_s1y;
	m_systemDmaData.m_c1z = vel[2]               - 1.5f * m_systemDmaData.m_s1z;
	m_systemDmaData.m_c1w = (m_ew-m_sw) / m_life - 1.5f * m_systemDmaData.m_s1w;


	// quadratic part
	m_systemDmaData.m_s2x = 0.0f;
	m_systemDmaData.m_s2y = 0.0f;
	m_systemDmaData.m_s2z = 0.0f;
	m_systemDmaData.m_s2w = 0.0f;

	m_systemDmaData.m_c2x = m_fx*0.5f*(60.0f*60.0f) - 1.5f * m_systemDmaData.m_s2x;
	m_systemDmaData.m_c2y = m_fy*0.5f*(60.0f*60.0f) - 1.5f * m_systemDmaData.m_s2y;
	m_systemDmaData.m_c2z = m_fz*0.5f*(60.0f*60.0f) - 1.5f * m_systemDmaData.m_s2z;
	m_systemDmaData.m_c2w = 0.0f                    - 1.5f * m_systemDmaData.m_s2w;


	// cubic part
	//m_systemDmaData.m_s3x = 0.0f;
	//m_systemDmaData.m_s3y = 0.0f;
	//m_systemDmaData.m_s3z = 0.0f;
	//m_systemDmaData.m_s3w = 0.0f;

	//m_systemDmaData.m_c3x = 0.0f - 1.5f * m_systemDmaData.m_s3x;
	//m_systemDmaData.m_c3y = 0.0f - 1.5f * m_systemDmaData.m_s3y;
	//m_systemDmaData.m_c3z = 0.0f - 1.5f * m_systemDmaData.m_s3z;
	//m_systemDmaData.m_c3w = 0.0f - 1.5f * m_systemDmaData.m_s3w;
	

	// colour
	if (m_mid_time >= 0.0f)		// if using mid-colour
	{
		float q0 = 1.0f / midTime;
		float q1 = 1.0f / (m_life - midTime);

		m_systemDmaData.m_c0r = ((float)m_mid_color.r - (float)m_start_color.r) * q0;
		m_systemDmaData.m_c0g = ((float)m_mid_color.g - (float)m_start_color.g) * q0;
		m_systemDmaData.m_c0b = ((float)m_mid_color.b - (float)m_start_color.b) * q0;
		m_systemDmaData.m_c0a = ((float)m_mid_color.a - (float)m_start_color.a) * q0;

		m_systemDmaData.m_c1r = (float)m_mid_color.r;
		m_systemDmaData.m_c1g = (float)m_mid_color.g;
		m_systemDmaData.m_c1b = (float)m_mid_color.b;
		m_systemDmaData.m_c1a = (float)m_mid_color.a;

		m_systemDmaData.m_c2r = ((float)m_end_color.r - (float)m_mid_color.r) * q1;
		m_systemDmaData.m_c2g = ((float)m_end_color.g - (float)m_mid_color.g) * q1;
		m_systemDmaData.m_c2b = ((float)m_end_color.b - (float)m_mid_color.b) * q1;
		m_systemDmaData.m_c2a = ((float)m_end_color.a - (float)m_mid_color.a) * q1;
	}
	else						// else suppress mid-colour
	{
		float q = 1.0f / m_life;

		m_systemDmaData.m_c1r = (float)m_start_color.r;
		m_systemDmaData.m_c1g = (float)m_start_color.g;
		m_systemDmaData.m_c1b = (float)m_start_color.b;
		m_systemDmaData.m_c1a = (float)m_start_color.a;

		m_systemDmaData.m_c2r = ((float)m_end_color.r - (float)m_start_color.r) * q;
		m_systemDmaData.m_c2g = ((float)m_end_color.g - (float)m_start_color.g) * q;
		m_systemDmaData.m_c2b = ((float)m_end_color.b - (float)m_start_color.b) * q;
		m_systemDmaData.m_c2a = ((float)m_end_color.a - (float)m_start_color.a) * q;
	}


	// rotation matrix, hardwired to the identity for now
	m_rotation.Identity();

	// temp test
	//m_rotation[0][0] =  0.8;
	//m_rotation[0][1] =  0.6;
	//m_rotation[1][0] = -0.6;
	//m_rotation[1][1] =  0.8;

	// invert rotation and apply to spatial params
	// leaving this code a bit shoddy and slow until full transition to new-style params
	Mth::Matrix mat;
	mat=m_rotation;
	mat.Transpose();
	Mth::Vector vec;

	vec[0] = m_systemDmaData.m_c0x + 1.5f * m_systemDmaData.m_s0x;
	vec[1] = m_systemDmaData.m_c0y + 1.5f * m_systemDmaData.m_s0y;
	vec[2] = m_systemDmaData.m_c0z + 1.5f * m_systemDmaData.m_s0z;
	vec[3] = 0;
	vec *= mat;
	m_systemDmaData.m_c0x = vec[0] - 1.5f * m_systemDmaData.m_s0x;
	m_systemDmaData.m_c0y = vec[1] - 1.5f * m_systemDmaData.m_s0y;
	m_systemDmaData.m_c0z = vec[2] - 1.5f * m_systemDmaData.m_s0z;

	vec[0] = m_systemDmaData.m_c1x + 1.5f * m_systemDmaData.m_s1x;
	vec[1] = m_systemDmaData.m_c1y + 1.5f * m_systemDmaData.m_s1y;
	vec[2] = m_systemDmaData.m_c1z + 1.5f * m_systemDmaData.m_s1z;
	vec[3] = 0;
	vec *= mat;
	m_systemDmaData.m_c1x = vec[0] - 1.5f * m_systemDmaData.m_s1x;
	m_systemDmaData.m_c1y = vec[1] - 1.5f * m_systemDmaData.m_s1y;
	m_systemDmaData.m_c1z = vec[2] - 1.5f * m_systemDmaData.m_s1z;
	
	vec[0] = m_systemDmaData.m_c2x + 1.5f * m_systemDmaData.m_s2x;
	vec[1] = m_systemDmaData.m_c2y + 1.5f * m_systemDmaData.m_s2y;
	vec[2] = m_systemDmaData.m_c2z + 1.5f * m_systemDmaData.m_s2z;
	vec[3] = 0;
	vec *= mat;
	m_systemDmaData.m_c2x = vec[0] - 1.5f * m_systemDmaData.m_s2x;
	m_systemDmaData.m_c2y = vec[1] - 1.5f * m_systemDmaData.m_s2y;
	m_systemDmaData.m_c2z = vec[2] - 1.5f * m_systemDmaData.m_s2z;

}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CParticleStream::AdvanceSeed(int num_places)
{
	for (int i=0; i<num_places; i++)
	{
		m_seed[0]  = m_seed[3]<<1 | ((m_seed[3]>>4 ^ m_seed[3]>>22) & 1);
		m_seed[1] ^= m_seed[0];
		m_seed[2] ^= m_seed[1];
		m_seed[3] ^= m_seed[2];
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CPs2ParticleNewFlat::GetNumParticles()
{
	int i, total=0;
	CParticleStream *p_stream;

	for (i=0, p_stream=mp_oldest_stream; i<m_num_streams; i++)
	{
		// add this stream to tally
		total += p_stream->m_num_particles;

		// step pointer within cyclic buffer
		p_stream++;
		if (p_stream == mp_stream + m_max_streams)
		{
			p_stream = mp_stream;
		}
	}

	return total;
}


} // Nx

