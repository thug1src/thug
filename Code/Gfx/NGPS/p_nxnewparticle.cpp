/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate5													**
**																			**
**	Module:			Gfx			 											**
**																			**
**	File name:		p_NxNewParticle.cpp										**
**																			**
**	Created by:		3/25/03	-	SPG											**
**																			**
**	Description:	PS2 new parametric particle system						**
*****************************************************************************/

#include <core/defines.h>
#include <gfx/ngps/p_NxNewParticle.h>
#include <gfx/ngps/nx/group.h>
#include <gfx/ngps/nx/texture.h>
#include <gfx/ngps/nx/dma.h>
#include <gfx/ngps/nx/vif.h>
#include <gfx/ngps/nx/gs.h>
#include <gfx/nx.h>

#include <gfx/NxTexMan.h>
#include <gfx/ngps/p_nxtexture.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Nx
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

void	CPs2NewParticle::plat_render( void )
{
	CParticleStream *p_stream;
	int i;

	//--------------------------------------------------------------------------------------------------------------------
	// process the streams
	if (m_params.m_EmitRate && (!m_emitting || (m_params.m_EmitRate != mp_newest_stream->m_rate)))
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
			mp_newest_stream->m_rate			= m_params.m_EmitRate;
			mp_newest_stream->m_interval		= 1.0f/m_params.m_EmitRate;
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
		m_emitting = m_params.m_EmitRate;
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
	if (mp_oldest_stream->m_oldest_age > m_params.m_Lifetime)
	{
		// work out number dead
		int particles_dead = (int)((mp_oldest_stream->m_oldest_age - m_params.m_Lifetime) * mp_oldest_stream->m_rate + 1.0f);

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

	// FOG
	Mth::Vector x = 0.5f * (m_params.m_BoxPos[0] + m_params.m_BoxPos[2]);
	x[3] = 1.0f;
	x *= NxPs2::render::WorldToFrustum;
	float f;
	if ((x[3] > NxPs2::render::FogNear) && NxPs2::render::EnableFog)	// Garrett: We have to check for EnableFog here because the VU1 code isn't
	{
		f = 1.0 + NxPs2::render::EffectiveFogAlpha * (NxPs2::render::FogNear/x[3] - 1.0f);
	}
	else
	{
		f = 1.0f;
	}
	m_systemDmaData.m_GScontext[21] = ((uint32)(int)(f*255.99f)) << 24;

	// set the group
	NxPs2::dma::SetList(NxPs2::sGroup::pParticles);

	// the system and streams will be loaded to a double-buffered input area of VUMem1
	// ref the system data, and include an unpack for the system and streams:
	NxPs2::dma::Tag(NxPs2::dma::ref, (sizeof(CSystemDmaData)>>4), (uint)&m_systemDmaData);
	NxPs2::vif::STCYCL(1,1);
	NxPs2::vif::UNPACK(0, V4_32, (sizeof(CSystemDmaData)>>4)+m_num_streams*2, REL, SIGNED, 0);

	// construct a packet with data for each stream
	NxPs2::dma::BeginTag(NxPs2::dma::cnt, 0);
	for (i=0,p_stream=mp_oldest_stream; i<m_num_streams; i++,p_stream++)
	{
		Dbg_MsgAssert(p_stream->m_num_particles<65536, ("particle limit reached (65536)"));

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

void	CPs2NewParticle::update_position( void )
{
	// convert 3-point -> PVA format
	float t1 = m_params.m_Lifetime * m_params.m_MidpointPct * 0.01f;
	float t2 = m_params.m_Lifetime;
	Mth::Vector x0,x1,x2,u,a_;

	x0    = m_params.m_BoxPos[0];
	x0[3] = m_params.m_Radius[0];
	x1    = m_params.m_BoxPos[1];
	x1[3] = m_params.m_Radius[1];
	x2    = m_params.m_BoxPos[2];
	x2[3] = m_params.m_Radius[2];

	if (m_params.m_UseMidpoint)
	{
		u  =  ( t2*t2*(x1 - x0) - t1*t1*(x2 - x0) ) / ( t1*t2*(t2 - t1) );
		a_ =  ( t1*(x2 - x0) - t2*(x1 - x0) ) / ( t1*t2*(t2 - t1) );
	}
	else
	{
		u  = ( x2 - x0 ) / t2;
		a_.Set(0,0,0,0);
	}

	m_systemDmaData.m_p0 = x0 - 1.5f * m_systemDmaData.m_s0;
	m_systemDmaData.m_p1 = u  - 1.5f * m_systemDmaData.m_s1;
	m_systemDmaData.m_p2 = a_ - 1.5f * m_systemDmaData.m_s2;
	m_systemDmaData.m_p0[3] = x0[3] - 1.5f * m_systemDmaData.m_s0[3];
	m_systemDmaData.m_p1[3] = u[3]  - 1.5f * m_systemDmaData.m_s1[3];
	m_systemDmaData.m_p2[3] = a_[3] - 1.5f * m_systemDmaData.m_s2[3];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2NewParticle::plat_update( void )
{
	if (m_params.m_LocalCoord)
	{
		update_position();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2NewParticle::plat_build( void )
{

	// initialise streams
	m_max_streams = 5;
	m_num_streams = 0;
	mp_stream = new CParticleStream[m_max_streams]; 
	mp_newest_stream = mp_stream + m_max_streams - 1;
	mp_oldest_stream = mp_stream;
	m_emitting = false;


	// giftag for gs context
	m_systemDmaData.m_GScontext[0] = 0x00008005;
	m_systemDmaData.m_GScontext[1] = 0x10000000;
	m_systemDmaData.m_GScontext[2] = 0x0000000E;
	m_systemDmaData.m_GScontext[3] = 0x00000000;

	// ALPHA_1 register
	uint64 AlphaReg = 0;
	uint8 fix=64;
	switch (m_params.m_BlendMode)
	{
		case 0x54628ed7:		// Blend
			AlphaReg = PackALPHA(0,1,0,1,0);
			break;
		case 0x02e58c18:		// Add
			AlphaReg = PackALPHA(0,2,0,1,0);
			break;
		case 0xa7fd7d23:		// Sub
		case 0xdea7e576:		// Subtract
			AlphaReg = PackALPHA(2,0,0,1,0);
			break;
		case 0x40f44b8a:		// Modulate
			AlphaReg = PackALPHA(1,2,0,2,0);
			break;
		case 0x68e77f40:		// Brighten
			AlphaReg = PackALPHA(1,2,0,1,0);
			break;
		case 0x18b98905:		// FixBlend
			AlphaReg = PackALPHA(0,1,2,1,fix);
			break;
		case 0xa86285a1:		// FixAdd
			AlphaReg = PackALPHA(0,2,2,1,fix);
			break;
		case 0x0d7a749a:		// FixSub
		case 0x0eea99ff:		// FixSubtract
			AlphaReg = PackALPHA(2,0,2,1,fix);
			break;
		case 0x90b93703:		// FixModulate
			AlphaReg = PackALPHA(1,2,2,2,fix);
			break;
		case 0xb8aa03c9:		// FixBrighten
			AlphaReg = PackALPHA(1,2,2,1,fix);
			break;
		case 0x515e298e:		// Diffuse
		case 0x806fff30:		// None
			AlphaReg = PackALPHA(0,0,0,0,0);
			break;
		default:
			Dbg_MsgAssert(0,("Illegal blend mode specified. Please use (fix)blend/add/sub/modulate/brighten or diffuse/none."));
			break;
	}
	m_systemDmaData.m_GScontext[6] = 0x00000042;
	*(uint64 *)&m_systemDmaData.m_GScontext[4] = AlphaReg;

	// TEX0_1 and TEX1_1 registers
	Nx::CTexture *p_texture;
	Nx::CPs2Texture *p_ps2_texture;
	mp_engine_texture = NULL;
	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( m_params.m_Texture );
	p_ps2_texture = static_cast<Nx::CPs2Texture*>( p_texture );
	if ( p_ps2_texture )
	{
		mp_engine_texture = p_ps2_texture->GetSingleTexture(); 
	}
	//printf("m_Texture = %08X\n", m_params.m_Texture);
	//printf("p_texture = %08X\n", p_texture);
	//printf("p_ps2_texture = %08X\n", p_ps2_texture);
	//printf("mp_engine_texture = %08X\n", mp_engine_texture);

	m_systemDmaData.m_GScontext[10] = 0x00000006;

	m_systemDmaData.m_GScontext[14] = 0x00000014;

	if (mp_engine_texture)
	{
		//printf("\n\nTEX0_1 = %08X%08X\n\n\n",
		//					(uint32)(mp_engine_texture->m_RegTEX0>>32),
		//					(uint32)(mp_engine_texture->m_RegTEX0));

		// texture coords
		m_systemDmaData.m_u0 = 8;
		m_systemDmaData.m_v0 = (mp_engine_texture->GetHeight()<<4) - 8;
		m_systemDmaData.m_u1 = (mp_engine_texture->GetWidth() <<4) - 8;
		m_systemDmaData.m_v1 = 8;
	}

	// TEST_1
	m_systemDmaData.m_GScontext[18] = 0x00000047;
	m_systemDmaData.m_GScontext[16] = 0x0005000B | m_params.m_AlphaCutoff<<4;

	// FOG
	m_systemDmaData.m_GScontext[22] = 0x0000000A;

	// giftag for particles
	m_systemDmaData.m_tagy = 0x60BB4000;
	m_systemDmaData.m_tagz = 0x00535312;



	// have already set m_Lifetime, which is called m_life in newflat

	// x-component will be overwritten by vu1 code, so can store midtime there
	*(float *)&m_systemDmaData.m_tagx = m_params.m_UseMidcolor ?
										m_params.m_Lifetime * m_params.m_ColorMidpointPct * 0.01f :
										0.0f;

	// and now a load of redundant duplication of data, which should later be removed...

	// convert 3-point -> PVA format
	float t1 = m_params.m_Lifetime * m_params.m_MidpointPct * 0.01f;
	float t2 = m_params.m_Lifetime;
	Mth::Vector x0,x1,x2,u,a_;

	//printf("\n\n\n\n\n%g,%g,%g\n%g,%g,%g\n\n\n\n\n",
	//	   m_params.m_BoxDims[0][3], m_params.m_BoxDims[1][3], m_params.m_BoxDims[2][3],
	//	   m_params.m_BoxPos[0][3], m_params.m_BoxPos[1][3], m_params.m_BoxPos[2][3]);

	x0    = m_params.m_BoxDims[0];
	x0[3] = m_params.m_RadiusSpread[0];
	x1    = m_params.m_BoxDims[1];
	x1[3] = m_params.m_RadiusSpread[1];
	x2    = m_params.m_BoxDims[2];
	x2[3] = m_params.m_RadiusSpread[2];

	if (m_params.m_UseMidpoint)
	{
		u  = ( t2*t2*(x1 - x0) - t1*t1*(x2 - x0) ) / ( t1*t2*(t2 - t1) );
		a_ = ( t1*(x2 - x0) - t2*(x1 - x0) ) / ( t1*t2*(t2 - t1) );
	}
	else
	{
		u  = ( x2 - x0 ) / t2;
		a_.Set(0,0,0,0);
	}

	m_systemDmaData.m_s0 = x0;
	m_systemDmaData.m_s1 = u;
	m_systemDmaData.m_s2 = a_;

	update_position();


	// colour
	if (m_params.m_UseMidcolor)
	{
		float q0 = 100.0f / (m_params.m_Lifetime * m_params.m_ColorMidpointPct);
		float q1 = 100.0f / (m_params.m_Lifetime * (100.0f - m_params.m_ColorMidpointPct));

		m_systemDmaData.m_c0[0] = ((float)m_params.m_Color[1].r - (float)m_params.m_Color[0].r) * q0;
		m_systemDmaData.m_c0[1] = ((float)m_params.m_Color[1].g - (float)m_params.m_Color[0].g) * q0;
		m_systemDmaData.m_c0[2] = ((float)m_params.m_Color[1].b - (float)m_params.m_Color[0].b) * q0;
		m_systemDmaData.m_c0[3] = ((float)m_params.m_Color[1].a - (float)m_params.m_Color[0].a) * q0;

		m_systemDmaData.m_c1[0] = (float)m_params.m_Color[1].r;
		m_systemDmaData.m_c1[1] = (float)m_params.m_Color[1].g;
		m_systemDmaData.m_c1[2] = (float)m_params.m_Color[1].b;
		m_systemDmaData.m_c1[3] = (float)m_params.m_Color[1].a;

		m_systemDmaData.m_c2[0] = ((float)m_params.m_Color[2].r - (float)m_params.m_Color[1].r) * q1;
		m_systemDmaData.m_c2[1] = ((float)m_params.m_Color[2].g - (float)m_params.m_Color[1].g) * q1;
		m_systemDmaData.m_c2[2] = ((float)m_params.m_Color[2].b - (float)m_params.m_Color[1].b) * q1;
		m_systemDmaData.m_c2[3] = ((float)m_params.m_Color[2].a - (float)m_params.m_Color[1].a) * q1;

	}
	else // else suppress mid-colour
	{
		float q = 1.0f / m_params.m_Lifetime;

		m_systemDmaData.m_c1[0] = (float)m_params.m_Color[0].r;
		m_systemDmaData.m_c1[1] = (float)m_params.m_Color[0].g;
		m_systemDmaData.m_c1[2] = (float)m_params.m_Color[0].b;
		m_systemDmaData.m_c1[3] = (float)m_params.m_Color[0].a;

		m_systemDmaData.m_c2[0] = ((float)m_params.m_Color[2].r - (float)m_params.m_Color[0].r) * q;
		m_systemDmaData.m_c2[1] = ((float)m_params.m_Color[2].g - (float)m_params.m_Color[0].g) * q;
		m_systemDmaData.m_c2[2] = ((float)m_params.m_Color[2].b - (float)m_params.m_Color[0].b) * q;
		m_systemDmaData.m_c2[3] = ((float)m_params.m_Color[2].a - (float)m_params.m_Color[0].a) * q;
	}



	// rotation matrix
	//m_rotation = m_params.m_RotMatrix;
	m_rotation.Identity();

	#if 0
	// invert rotation and apply to spatial params
	// leaving this code a bit shoddy and slow until full transition to new-style params
	Mth::Matrix mat;
	mat=m_rotation;
	mat.Transpose();
	Mth::Vector vec;

	vec = m_systemDmaData.m_p0 + 1.5f * m_systemDmaData.m_s0;
	vec *= mat;
	m_systemDmaData.m_p0 = vec - 1.5f * m_systemDmaData.m_s0;

	vec = m_systemDmaData.m_p1 + 1.5f * m_systemDmaData.m_s1;
	vec *= mat;
	m_systemDmaData.m_p1 = vec - 1.5f * m_systemDmaData.m_s1;

	vec = m_systemDmaData.m_p2 + 1.5f * m_systemDmaData.m_s2;
	vec *= mat;
	m_systemDmaData.m_p2 = vec - 1.5f * m_systemDmaData.m_s2;
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CPs2NewParticle::plat_destroy( void )
{
	
	// Bit of a patch, but need to wait until particle system DMA packet has
	// been used before we can delete it, otherwise memory might get corrupted
	// in normal gameplay particle systems are rarely created
	// but in "FireFight" it shows up a lot more.
	// This really should be handled at a lower level
	Nx::CEngine::sFinishRendering();
	
	
	if( mp_stream )
	{
		delete [] mp_stream;
	}
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Nx




