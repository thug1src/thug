///*****************************************************************************
//**																			**
//**					   	  Neversoft Entertainment							**
//**																		   	**
//**				   Copyright (C) 1999 - All Rights Reserved				   	**
//**																			**
//******************************************************************************
//**																			**
//**	Project:		Skate3      											**
//**																			**
//**	Module:			GFX			  											**
//**																			**
//**	Created:		08/16/01	dc											**
//**																			**
//**	File name:		p_particle.h               								**
//**																			**
//**	Description:	BGC particle system interface              				**
//**																			**
//*****************************************************************************/
//
//#ifndef __P_PARTICLE_H
//#define __P_PARTICLE_H
//
///*****************************************************************************
//**							  	  Includes									**
//*****************************************************************************/
//
//#ifndef __CORE_DEFINES_H
//#include <core/defines.h>
//#endif
//
//#include <core/lookuptable.h>
//
//namespace Particle
//{
//
///*****************************************************************************
//**								   Defines									**
//*****************************************************************************/
//
///*****************************************************************************
//**							Class Definitions								**
//*****************************************************************************/
//
//struct parametric_particle
//{
//	NsVector	start_pos;
//	NsVector	initial_velocity;
//	float		t;			// [0.0, 1.0]
//};
//
//nBaseClass( NsParticleAtomic )
//{
//	Dbg_BaseClass( NsParticleAtomic );
//
//	public:
//
//								NsParticleAtomic( int num_particle, NsMaterial* p_material );
//								~NsParticleAtomic();
//
//	void						update( float delta_t );
//	void						SetNumParticles( int num );
//	void						setMaterial( NsMaterial* p_material );
//	void						SetEmitterSize( float w, float h );
//	void						SetEmitterAngle( float angle );
//	void						setParticleColors( uint32* start, uint32* end );
//	uint32						getParticleColors( uint32 index );
//	void						setParticleSpeed( float min, float max, float damping );
//	void						setParticleFlightTime( float t );
//	void						setParticleForce( NsVector& f );
//	void						setParticleSize( float size, float growth, float aspect_ratio );
//	void						draw( void );
//	NsFrame*					getFrame( void )		{ return &m_frame; }
//	void						setVisible( bool vis )	{ m_visible = vis; }
//
//	enum EParticleFlags
//	{
//		PARTICLE_FLAG_DIRTY			= 0x01,
//		PARTICLE_FLAG_BBOX_DIRTY	= 0x02
//	};
//
//
//	private:
//
//	unsigned int				m_flags;
//	NsBBox						m_bbox;
//	NsMaterial*					mp_material;
//	NsFrame						m_frame;
//	struct parametric_particle*	mp_particles;
//	int							m_initial_num_particles;
//	int							m_num_particles;			// This can be set dynamically to *less* than the initial number.
//	bool						m_visible;
//	float						m_time_since_last_draw;
//	float						m_emitter_width;
//	float						m_emitter_height;
//	float						m_emitter_angle;
//	uint32						m_start_color;
//	uint32						m_end_color;
//	float						m_gx_start_color_r, m_gx_start_color_g, m_gx_start_color_b, m_gx_start_color_a;
//	float						m_gx_delta_color_r, m_gx_delta_color_g, m_gx_delta_color_b, m_gx_delta_color_a;
//	float						m_min_speed;
//	float						m_variation_speed;
//	float						m_damping;
//	float						m_flight_time;
//	float						m_inv_flight_time;
//	NsVector					m_force;
//	float						m_size;
//	float						m_growth;
//	float						m_aspect_ratio;
//
//	void						seedParticle( int p, float t_max );
//	void						figureBoundingBox( void );
//	bool						cull( void );
//};
//
//
//
///*****************************************************************************
//**							 Private Declarations							**
//*****************************************************************************/
//
//
///*****************************************************************************
//**							  Private Prototypes							**
//*****************************************************************************/
//
//
///*****************************************************************************
//**							  Public Declarations							**
//*****************************************************************************/
//
///*****************************************************************************
//**							   Public Prototypes							**
//*****************************************************************************/
//
//void SetParticleAtomicCamera( NsCamera* p_cam );
//
//
//
///*****************************************************************************
//**								Inline Functions							**
//*****************************************************************************/
//
//} // namespace Particle
//
//#endif // __P_PARTICLE_H
//
