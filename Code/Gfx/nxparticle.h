///////////////////////////////////////////////////////////////////////////////
// NxParticle.h



#ifndef	__GFX_NXPARTICLE_H__
#define	__GFX_NXPARTICLE_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <gfx/image/imagebasic.h>
#include <gfx/nxtexture.h>
#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif
#include <sk/objects/movingobject.h>



namespace Script
{

// Forward declarations
class CScript;

}

namespace Nx
{

// Forward declarations
class CParticleEntry;

//////////////////////////////////////////////////////////////////////////////
// The CParticle class is the platform independent abstract base class
// of the platform specific CParticle classes
class	CParticle : public Obj::CMovingObject
{
public:
							CParticle();
							CParticle( uint32 checksum );
							CParticle( uint32 checksum, int maxParticles );
	virtual					~CParticle();

	void					process( float delta_time );
	void					render( void );

	void					emit( int count );

	void					set_pos( float x, float y, float z )			{ m_pos[X] = x; m_pos[Y] = y; m_pos[Z] = z; }
	void					set_speed_range( float min, float max )			{ m_speed_min = min; m_speed_max = max; }
	void					set_emit_range( float width, float height )		{ m_emit_w = width; m_emit_h = height; }
	void					set_emit_angle_spread( float spread )			{ m_emit_angle_spread = spread; }
	void					set_random_angle( bool random_angle )			{ m_random_angle = random_angle; }
	void					set_circular_emit( bool circular_emit )			{ m_circular_emit = circular_emit; }
	void					set_force( float x, float y, float z )			{ m_fx = x; m_fy = y; m_fz = z; }
	void					set_particle_start_size( float w, float h )		{ m_sw = w; m_sh = h; }
	void					set_particle_end_size( float w, float h )		{ m_ew = w; m_eh = h; }
	void					set_particle_life( float min, float max )		{ m_life_min = min; m_life_max = max; m_life_set = true; }
	void					set_emit_target( float x, float y, float z )	{ m_tx = x; m_ty = y; m_tz = z; }
	void					set_emit_angle( float x, float y, float z )		{ m_ax = x; m_ay = y; m_az = z; }
	void					set_emit_rate( float rate )						{ m_emit_rate = rate; }

	void					set_checkum(uint32 checksum) {m_checksum = checksum;}

	void					delete_when_empty( void )						{ m_delete_when_empty = true; }

	bool					CallMemberFunction (uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript);	// Call a member function based on the checksum of the function name

	void					set_emit_script( uint32 checksum, Script::CStruct* pParams );
	void					set_update_script( uint32 checksum, Script::CStruct* pParams );

	void					SetActive( bool active );
	bool					IsActive()										{ return m_active; }
	
	void					SetEmitting( bool emitting )					{ m_emitting = emitting; }
	bool					IsEmitting()									{ return m_emitting; }


	void					SetPerm( bool perm )							{ m_perm = perm; }
	bool					IsPerm()										{ return m_perm; }

	void					SetNumParticles( int num )						{ m_num_particles = num; }
	int						GetNumParticles( void )							{ return m_num_particles; }

	void 					Refresh();		


protected:
	// Extensions for Mike's system.
	float					m_emit_rate;			// The rate of emission - 1.0 = 1 per frame.

	// Member variables (protected so the p-classes can access them)
	int						m_max_particles;		// Maximum size of particle array.
	int						m_num_particles;		// Current number of active particles.
		
	float					m_speed_min;			// Initial Speed setting.
	float					m_speed_max;
	float					m_emit_w, m_emit_h;		// Emitter dimensions.
	float					m_emit_angle_spread;	// Angle spread from 0 to 1.
	
	// Should be changed to a flag value...
	bool					m_random_angle;			// true means use a random angle, regardless of the emit pos.
	bool					m_circular_emit;		// true means emit in a circle.

	float					m_fx, m_fy, m_fz;		// Initial Force values. fy is gravity, will not dec to 0.
	float					m_sw, m_sh;				// Initial width & height.
	float					m_ew, m_eh;				// Initial width & height.
	float					m_life_min, m_life_max;	// Life min & max

	float					m_tx, m_ty, m_tz;		// Target position.
	float					m_ax, m_ay, m_az;		// Angle rotations.
	
	uint32					m_checksum;

	bool					m_delete_when_empty;	// When m_num_particles == 0, this system will self-delete.

	CParticleEntry*			mp_particle_array;		// Pointer to array of particles.

	Script::CScript*		mp_emit_script;
	Script::CScript*		mp_update_script;

	// parameters that will be passed to the emit/update scripts
	// generally used for passing the ID of a moving object
	Script::CStruct*		mp_params;
	bool					m_active;
	bool					m_emitting;

	float					m_mid_time;					// Time of the mid color.

	bool					m_life_set;

	bool					m_end_set;

	bool					m_perm;						// true if not deleted between levels

	// The virtual functions have a stub implementation.
private:
	virtual void			plat_render( void );
	virtual void			plat_get_position( int entry, int list, float * x, float * y, float * z );
	virtual void			plat_set_position( int entry, int list, float x, float y, float z );
	virtual void			plat_add_position( int entry, int list, float x, float y, float z );
	virtual int				plat_get_num_vertex_lists( void );
	virtual int				plat_get_num_particle_colors( void );
	virtual void			plat_set_sr( int entry, uint8 value );
	virtual void			plat_set_sg( int entry, uint8 value );
	virtual void			plat_set_sb( int entry, uint8 value );
	virtual void			plat_set_sa( int entry, uint8 value );
	virtual void			plat_set_mr( int entry, uint8 value );
	virtual void			plat_set_mg( int entry, uint8 value );
	virtual void			plat_set_mb( int entry, uint8 value );
	virtual void			plat_set_ma( int entry, uint8 value );
	virtual void			plat_set_er( int entry, uint8 value );
	virtual void			plat_set_eg( int entry, uint8 value );
	virtual void			plat_set_eb( int entry, uint8 value );
	virtual void			plat_set_ea( int entry, uint8 value );
	virtual void			plat_set_active( bool active );
	virtual void			plat_build_path( void );

	void					set_defaults( void );
};

//////////////////////////////////////////////////////////////////////////////
// Particle system entry. Does very little other than hold information. The controller does all the work.
class CParticleEntry
{
public:
							CParticleEntry();
							~CParticleEntry();

//							float					m_x, m_y, m_z;				// Current x, y, z positions.
							float					m_vx, m_vy, m_vz;			// Current velocity.
							float					m_fx, m_fy, m_fz;			// Current force.
							float					m_sw, m_sh;					// Start Width & Height.
							float					m_ew, m_eh;					// EndWidth & Height.
							float					m_time, m_life;				// Current time & life of particle.
protected:
private:
	friend CParticle;
};

}

#endif // 


