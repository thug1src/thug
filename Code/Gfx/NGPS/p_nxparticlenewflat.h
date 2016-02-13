//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticle.h
//* OWNER:          Paul Robinson
//* CREATION DATE:  3/27/2002
//****************************************************************************

#ifndef	__GFX_P_NX_PARTICLENEWFLAT_H__
#define	__GFX_P_NX_PARTICLENEWFLAT_H__
    
#include "gfx/ngps/p_nxparticle.h"
                   
namespace Nx
{

class CParticleSpan
{
public:
	float					m_sx,m_sy,m_sz;			// Start, mid & end points of span path.
	float					m_mx,m_my,m_mz;			// This should really be the coefficients you need to
	float					m_ex,m_ey,m_ez;			// describe the path, not the raw position data.
	unsigned char			m_sr,m_sg,m_sb,m_sa;	// Start color
	unsigned char			m_er,m_eg,m_eb,m_ea;	// End color
};

class CParticleStream
{
public:
	int						m_num_particles;
	float					m_rate;
	float					m_interval;
	float					m_oldest_age;
	uint32					m_seed[4];
	void					AdvanceSeed(int num_places);
};

class CSystemDmaDataFlat
{
public:
	uint32					m_GScontext[20];
	uint32					m_u0,  m_v0,  m_u1,  m_v1;
	float					m_c0x, m_c0y, m_c0z, m_c0w;
	float					m_c1x, m_c1y, m_c1z, m_c1w;
	float					m_c2x, m_c2y, m_c2z, m_c2w;
	float					m_s0x, m_s0y, m_s0z, m_s0w;
	float					m_s1x, m_s1y, m_s1z, m_s1w;
	float					m_s2x, m_s2y, m_s2z, m_s2w;
	float					m_c0r, m_c0g, m_c0b, m_c0a;
	float					m_c1r, m_c1g, m_c1b, m_c1a;
	float					m_c2r, m_c2g, m_c2b, m_c2a;
	uint32					m_tagx,m_tagy,m_tagz,m_tagw;
	Mth::Matrix				m_matrix;
} nAlign(128);

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/////////////////////////////////////////////////////////////////////////////////////
//
// Here's a machine specific implementation of the CParticle
    
class CPs2ParticleNewFlat : public CParticle
{
public:
							CPs2ParticleNewFlat();
							CPs2ParticleNewFlat( uint32 checksum, int max_streams, uint32 texture_checksum, uint32 blendmode_checksum, int fix );
	virtual 				~CPs2ParticleNewFlat();

	int						GetNumParticles();

//	NxPs2::sParticleSystem*	GetEngineParticle( void )	{ return mp_engine_particle; }
private:					// It's all private, as it is machine specific
	void					plat_render( void );
	void					plat_get_position( int entry, int list, float * x, float * y, float * z );
	void					plat_set_position( int entry, int list, float x, float y, float z );
	void					plat_add_position( int entry, int list, float x, float y, float z );
	int						plat_get_num_vertex_lists( void );
	int						plat_get_num_particle_colors( void );

	void					plat_set_sr( int entry, uint8 value );
	void					plat_set_sg( int entry, uint8 value );
	void					plat_set_sb( int entry, uint8 value );
	void					plat_set_sa( int entry, uint8 value );
	void					plat_set_mr( int entry, uint8 value );
	void					plat_set_mg( int entry, uint8 value );
	void					plat_set_mb( int entry, uint8 value );
	void					plat_set_ma( int entry, uint8 value );
	void					plat_set_er( int entry, uint8 value );
	void					plat_set_eg( int entry, uint8 value );
	void					plat_set_eb( int entry, uint8 value );
	void					plat_set_ea( int entry, uint8 value );
	void					plat_build_path( void );
		
	Image::RGBA				m_start_color;				// Start color for each corner.
	Image::RGBA				m_mid_color;				// Mid color for each corner.
	Image::RGBA				m_end_color;				// End color for each corner.
	NxPs2::SSingleTexture*	mp_engine_texture;

	uint64					m_blend;

	int						m_max_streams;			// Maximum number of particle streams.
	int						m_num_streams;			// Number of active streams.
	int						m_num_spans;			// Number of span segments.

	CParticleSpan*			mp_span;
	CParticleStream*		mp_stream;
	CParticleStream*		mp_newest_stream;
	CParticleStream*		mp_oldest_stream;
	float					m_life;
	bool					m_emitting;


	Mth::Matrix				m_rotation;

	// dma data
	CSystemDmaDataFlat			m_systemDmaData;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // Nx


#endif 


