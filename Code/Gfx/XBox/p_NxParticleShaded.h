#ifndef	__GFX_P_NX_PARTICLESHADED_H__
#define	__GFX_P_NX_PARTICLESHADED_H__
    
#include "gfx/nxparticle.h"
#include "gfx/xbox/nx/particles.h"
                   
namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CXboxParticleShaded : public CParticle
{
	public:
							CXboxParticleShaded();
							CXboxParticleShaded( uint32 checksum, int max_particles = 256, uint32 texture_checksum = 0, uint32 blendmode_checksum = 0, int fix = 128 );
	virtual 				~CXboxParticleShaded();

	NxXbox::sParticleSystem	*GetEngineParticle( void )	{ return mp_engine_particle; }


	private:				// It's all private, as it is machine specific
	virtual void			plat_render( void );
	virtual void			plat_get_position( int entry, int list, float *x, float *y, float *z );
	virtual void			plat_set_position( int entry, int list, float x, float y, float z );
	virtual void			plat_add_position( int entry, int list, float x, float y, float z );
	virtual int				plat_get_num_particle_colors( void );
	virtual int				plat_get_num_vertex_lists( void )		{ return 1; }

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

	Image::RGBA				m_start_color[4];			// Start color for each corner.
	Image::RGBA				m_mid_color[4];				// Mid color for each corner.
	Image::RGBA				m_end_color[4];				// End color for each corner.
	
	float*					mp_vertices;
	NxXbox::sParticleSystem	*mp_engine_particle;

};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // Nx

#endif 



